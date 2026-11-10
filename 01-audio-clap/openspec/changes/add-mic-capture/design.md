# Design

## Context

See proposal.md for motivation. The firmware after board bringup is a single `app_main` loop that advances the LED and logs a count once per second via `vTaskDelay`, plus a `status_led` module written as a namespace with free functions and the GPIO as a module constant. The bringup design anticipated that this loop would be replaced once the firmware had real work, and it recorded one constraint that carries over: a log call can stall when no host drains the USB console, so audio code must not log from its capture path.

Hardware facts that shape the approach, from the ESP-IDF v6.1 tree the project builds against:

- The ESP32-S3 has two ADC units. ADC1 is GPIO1–GPIO10 (channel = GPIO − 1). ADC2 is GPIO11–GPIO20.
- ADC2 cannot be used in continuous (DMA) mode on the S3. The driver rejects it with `ESP_ERR_INVALID_ARG` because of an errata entry, unless `CONFIG_ADC_CONTINUOUS_FORCE_USE_ADC2_ON_C3_S3` is forced on. ADC2 is also shared with Wi-Fi. So audio capture is limited to ADC1 regardless of Wi-Fi.
- The S3 has one ADC digital controller and one DMA path. `adc_continuous_start` takes an exclusive lock on the unit, so only one continuous driver instance can run. Several inputs are captured by putting several channels in one scan pattern, not by creating several drivers.
- Continuous mode samples at a hardware-timed rate between 611 Hz and 83,333 Hz and delivers results by DMA in frames. Each result is 4 bytes: a 12-bit value plus the unit and channel it came from. The driver keeps 5 internal DMA frame buffers and pushes each completed frame into a FreeRTOS ring buffer that `adc_continuous_read` blocks on.
- The MAX4466 outputs an analog signal biased at about half its supply (~1.65 V at 3.3 V) and swinging around it. At its highest gain its bandwidth is only a few kilohertz; there is no anti-alias filter on the path.
- `main` declares no requirements and so receives every component implicitly, which is how bringup includes `esp_flash.h` without naming `spi_flash`. Declaring `esp_adc` would end that and force `spi_flash` to be declared too, for no functional gain; the module stays in `main` and the convention is kept.

## Goals / Non-Goals

**Goals:**

- A capture module whose output (frames of signed samples, one buffer per microphone) does not change when the consumer changes from statistics logging to clap detection or to a model.
- Make wrong wiring and wrong gain obvious from the console without extra tooling.
- Keep the module as small as `status_led`: it moves samples and removes bias, nothing else.

**Non-Goals:**

- Choosing a detection algorithm. The module hands out frames; what happens to them is the next change.
- Per-sample timestamps or inter-microphone alignment beyond what the scan pattern gives for free.
- Surviving a consumer that is permanently slower than the frame rate. That is a bug in the consumer, not something the module should hide.

## Decisions

### Continuous (DMA) mode over oneshot

Oneshot reads one value per call and leaves the spacing between samples to software; at 16 kHz with a 10 ms FreeRTOS tick that spacing would be set by scheduler jitter, and the CPU would be busy for every sample. Continuous mode makes the hardware the clock: samples are evenly spaced, DMA fills a frame with no CPU involvement, and the consumer wakes once per frame. A timer-driven oneshot loop was rejected as a hand-rolled, less accurate version of what continuous mode already is.

### 16 kHz, 256-sample frames, 4-frame store

A clap is a broadband burst that an energy detector can catch at 8 kHz, so 16 kHz is not required by the clap. It was chosen because it is the standard rate for speech and keyword-spotting models, which this project is named for, and because the MAX4466's own roll-off is the only thing preventing aliasing, which argues against going lower. The cost difference between 8 and 16 kHz is negligible on this chip.

256 samples at 16 kHz is 16 ms per frame: short enough that a reaction to a clap is not perceptibly delayed, long enough that the consumer wakes only 62 times per second. It is also a convenient block size for a detector, so one `read` can be one analysis block without re-buffering. The driver's `conv_frame_size` is therefore 256 × 4 bytes per microphone, and `max_store_buf_size` is 4 frames (64 ms of slack). Both are constants in the module.

### One class, any number of microphones

`MicArray` is a class rather than a namespace module like `status_led` because the set of pins is a runtime parameter, not a board constant. It is one object for all microphones rather than one object per microphone because the hardware has one continuous controller: each configured GPIO becomes one channel in the scan pattern, the controller's rate is set to 16 kHz × N so each channel still sees 16 kHz, and the frame holds 256 × N results. The N ≤ 5 limit from the 83.3 kHz cap is enforced at construction. A per-microphone class that owned the driver was rejected because a second instance could never start; a capture engine plus per-microphone view objects was rejected as two abstractions for what is currently one microphone.

With two microphones the samples are interleaved by the scan, so microphone B's sample lags A's by 1/(16 kHz × N). That offset is deterministic and small; it is noted here so that any later direction-finding work knows it exists.

### Map GPIOs with the driver, reject anything but ADC1

Each GPIO is resolved with `adc_continuous_io_to_channel`. A result other than `ADC_UNIT_1`, or a non-ADC pin, is reported with the GPIO number and the reason and then aborted via `ESP_ERROR_CHECK`, matching how `status_led` and the IDF examples treat configuration errors. Hard-coding the GPIO-to-channel table was rejected because the driver already has it.

### Demultiplex by the result's channel tag

Each 4-byte DMA result carries its channel. The module routes each result to the microphone buffer whose channel matches, instead of assuming pattern order. That is one compare per sample and turns a misconfiguration into a visible error rather than swapped microphones. A result tagged with an unexpected channel is counted and the frame is still delivered, so a rare glitch does not stall capture; the count is exposed for diagnostics. The newer `adc_continuous_parse_data` helper was rejected because it expands every result into a 16-byte struct for no benefit with a single, known channel set.

### Remove DC bias per microphone, emit `int16_t`

Each microphone has its own bias point, so the module tracks a running mean per channel and subtracts it, producing signed samples centered on zero. This belongs in the module because the bias is a property of the MAX4466, not of any detector. The tracked raw bias is exposed so `app_main` can log it as a wiring check (a connected MAX4466 reads roughly mid-scale, about 2048). Emitting raw unsigned codes and leaving bias removal to the consumer was rejected because every consumer would need it.

The running mean is an exponential average with a long time constant (on the order of a second), updated once per frame from the frame's mean rather than per sample, so it costs almost nothing and does not track the audio itself.

### 12 dB attenuation, no calibration

`ADC_ATTEN_DB_12` gives roughly 0–3.1 V of input range, which covers a MAX4466 swinging around 1.65 V. Calibration to millivolts is skipped: relative amplitude is all that audio processing needs, and the calibration scheme would add code and a second handle for nothing.

### `flush_pool` on, and the consumer is told about overflow

When the ring buffer is full the driver, with `flags.flush_pool` set, discards its contents and stores the newest frame; without it, the newest frame is dropped and `read` keeps returning progressively staler audio. For a detector, fresh audio after a stall is always what is wanted. The driver's `on_pool_ovf` callback runs in interrupt context, so the module only increments a counter there and exposes it; nothing else happens in the ISR.

### Blocking `read`, consumer owns the task

`MicArray::read` blocks on `adc_continuous_read` for exactly one frame and fills one caller-supplied buffer per microphone. The consumer runs in whatever task calls it, so no task, queue, or callback is created by the module. An ISR callback per frame was rejected because of the IRAM and no-logging constraints it imposes on consumer code; a capture task with a queue was rejected as moving parts that nothing yet needs.

### Static memory only in project code

Project code does not allocate on the heap. `MicArray` holds its microphones in a fixed array sized by the compile-time maximum (`SOC_ADC_SAMPLE_FREQ_THRES_HIGH / 16 kHz` = 5) and reads each frame through one fixed 256-result buffer, routing chunk by chunk, so its footprint does not grow with the microphone count. The `MicArray` object and the frame buffers are `static` in `app_main`. The ESP-IDF continuous driver still allocates its ring buffer, DMA buffers, and descriptors inside `adc_continuous_new_handle`, once, in `start()`; it has no static-allocation API, and replacing it with HAL-level code was rejected as far beyond this change. The same already holds for the `led_strip` RMT driver.

### `app_main` paced by frames, statistics kept in `app_main`

The ring buffer only holds 64 ms, so `app_main` can no longer sleep for a second. The loop inverts: `read` paces it at 62 Hz, and every 62 frames it advances the LED, logs the heartbeat count, and logs the statistics. The statistics (minimum, maximum, RMS over the second, plus the bias) live in a small helper in `app_main`, not in `MicArray`, so that the module stays a frame source and the clap detector can replace the helper without touching it. Minimum and maximum are tracked over the whole second rather than the last frame because a 16 ms clap would almost never coincide with a once-per-second snapshot.

RMS is computed from a 64-bit sum of squares over 256 × 62 samples, which cannot overflow, with one square root per second.

## Risks / Trade-offs

- **[A log call stalls because no host drains the USB console, and frames pile up]** → The store holds 4 frames and `flush_pool` discards older ones, so capture recovers with fresh audio once the log call returns. The heartbeat logs only once per second, so the exposure is small. Audio code itself never logs in the read path.
- **[ADC noise makes the quiet-room RMS higher than expected]** → The S3's ADC has a few least-significant bits of noise; that is fine for an energy detector. The logged RMS gives the real noise floor to design the detector threshold against. The S3's hardware IIR filters stay unused until there is evidence they are needed.
- **[Gain pot set too high clips on a clap]** → The logged minimum or maximum pinned at the ends of the range shows this directly; the pot is turned down until a clap no longer pins.
- **[Microphone power coupling from the LED]** → Already mitigated by the LED brightness cap chosen in board bringup. If the logged RMS changes with LED color, that is the diagnosis.
- **[Wrong GPIO wired, or the S3-Zero does not break out the chosen pin]** → Rejected ADC2 or non-ADC pins abort at boot with the reason. A correct but unconnected pin shows a bias far from mid-scale or a flat minimum and maximum in the log.
- **[`conv_frame_size` grows with N and may exceed a single DMA descriptor]** → The driver splits a frame across descriptors itself; nothing to do.
- **[A later consumer takes longer than 16 ms per frame]** → Overflow is counted and visible, not silent. A consumer that cannot keep up must be fixed or the frame size raised; the module does not hide it.

## Migration Plan

No configuration or partition changes. The new code is additive except for the `app_main` loop; rollback is restoring `main/01-audio-clap.cpp` and `main/CMakeLists.txt` and removing the module files.
