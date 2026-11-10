# Proposal

## Why

The board bringup change proved flashing, logging, and the LED, but the firmware still has no audio input, so nothing on the path from the MAX4466 microphone to the ESP32-S3's ADC has been exercised. Clap detection needs a steady stream of microphone samples first, and the capture layer is where the hardware's constraints (which ADC, what rate, how samples arrive) get settled.

## What Changes

- Add a `MicArray` module that captures audio from one or more MAX4466 microphones on ADC1 pins, configured by GPIO number with GPIO1 as the default, and delivers fixed-size frames of signed samples with each microphone's DC bias removed.
- Validate the microphone pins at startup: a pin that is not an ADC1 input stops the firmware with a logged reason instead of producing silent garbage.
- Rework the `app_main` loop so it is paced by audio frames rather than by a one-second delay, while keeping the existing LED and log heartbeat at about once per second.
- Log per-microphone signal statistics (minimum, maximum, RMS, and the tracked bias) about once per second, so a connected monitor shows whether the microphone is wired, how loud the room is, and whether a clap registers. This is also how the module's gain pot gets tuned.

Out of scope:

- Clap detection itself, and any processing of the frames beyond the logged statistics.
- ADC calibration to millivolts. Audio processing only needs relative amplitude.
- Wiring a second microphone. The module supports several, but this change is verified with one.
- Any Wi-Fi or ADC2 use.

## Capabilities

### New Capabilities

- `mic-capture`: The firmware captures audio from MAX4466 microphones as a stream of fixed-size sample frames, validates the microphone wiring at startup, and reports signal levels on the console.

### Modified Capabilities

None. The `board-config` heartbeat log and the `status-led` heartbeat keep their observable behavior: an increasing count logged about once per second and the LED cycling red, green, blue. Only the loop's pacing mechanism changes, which is an implementation detail.

## Impact

- **Code**: `main/mic_array.{h,cpp}` is added. `main/01-audio-clap.cpp` changes: the heartbeat loop becomes a frame-driven loop that also accumulates and logs microphone statistics. `main/CMakeLists.txt` lists the new source.
- **Configuration**: None. The ADC continuous driver's defaults are sufficient; no `sdkconfig` change is needed.
- **Dependencies**: `esp_adc` is part of ESP-IDF and reaches `main` through its implicit requirements, as `spi_flash` and `led_strip` already do; no managed component is added and nothing is declared.
- **Hardware**: Verification needs a MAX4466 module wired to GPIO1, 3V3, and GND on the ESP32-S3-Zero. The driver claims ADC1's digital controller and DMA path, so no other continuous ADC user can coexist with it.
