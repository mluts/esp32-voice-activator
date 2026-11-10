# Tasks

## 1. Build dependency

- [x] 1.1 Add the new `mic_array.cpp` source to `main/CMakeLists.txt` without declaring any requirements; verify `idf.py build` still succeeds with an empty `mic_array.cpp` before any module code is written.

## 2. MicArray module

- [ ] 2.1 Add `main/mic_array.h` declaring `MicArray` with a constructor taking the microphone GPIO list (default `{1}`), `start()`, a blocking `read()` that fills one 256-sample `int16_t` buffer per microphone, and accessors for each microphone's raw bias, the overflow count, and the unexpected-channel count; put the sample rate, frame size, and store size in named constants; verify `idf.py build` succeeds.
- [ ] 2.2 Implement GPIO resolution in `main/mic_array.cpp` using `adc_continuous_io_to_channel`, aborting via `ESP_ERROR_CHECK` with a log line naming the GPIO and reason for any non-ADC1 pin, and for more than 5 microphones; verify by temporarily constructing with GPIO11 and with GPIO21 that the build runs and the boot log names the rejected pin before aborting, then restore GPIO1.
- [ ] 2.3 Implement `start()`: create the continuous handle with `conv_frame_size` = 256 × N × 4, `max_store_buf_size` = 4 frames, `flush_pool` set; configure a scan pattern with one entry per microphone on `ADC_UNIT_1`, `ADC_ATTEN_DB_12`, 12 bits, at 16 kHz × N; register an `on_pool_ovf` callback that only increments a counter; start the driver; verify `idf.py build` succeeds and a boot with GPIO1 logs no ADC driver errors.
- [ ] 2.4 Implement `read()`: block on `adc_continuous_read` for one frame, route each 4-byte result to the microphone whose channel matches its tag (counting any unexpected tag), update each microphone's running bias from the frame mean, and write bias-removed `int16_t` samples; verify by a temporary debug build that prints the first few samples and the bias of one frame, showing the bias near 2048 and samples near zero with a connected microphone, then remove the debug output.

## 3. Application loop

- [ ] 3.1 Add a small statistics helper in `main/01-audio-clap.cpp` that accumulates per-microphone minimum, maximum, and a 64-bit sum of squares over a one-second window of frames, and formats one log line per microphone with min, max, RMS, and the raw bias; verify `idf.py build` succeeds.
- [ ] 3.2 Replace the `vTaskDelay` heartbeat loop with a loop that calls `MicArray::read` each iteration and, every 62 frames, advances the LED, logs the heartbeat count, logs the statistics, and resets the window; verify `idf.py build` succeeds and report the app size and free space in the 1 MB partition using `idf.py size`.

## 4. Verification on the board

These tasks need the ESP32-S3-Zero connected by USB with a MAX4466 wired to GPIO1 (OUT), 3V3 (VCC), and GND.

- [ ] 4.1 Run `idf.py -p <port> flash monitor`; verify the boot report still appears, no ADC driver errors are logged, and heartbeat lines keep arriving about once per second with an increasing count while the LED cycles red, green, blue, for at least 10 seconds.
- [ ] 4.2 Watch the statistics lines in a quiet room for at least 5 seconds; verify the bias is roughly mid-scale (about 2048), the RMS is small but not zero, and min and max are both present with opposite signs. Record the quiet-room RMS in this task's description for the detector change to use.
- [ ] 4.3 Clap near the microphone and watch the next statistics line; verify its min and max are several times larger in magnitude than the quiet-room values. If either pins at the end of the sample range, turn the gain pot down and repeat until a clap no longer pins.
- [ ] 4.4 Disconnect the microphone's OUT wire from GPIO1 and tie the pin to GND; verify the logged bias moves far from mid-scale and min and max collapse to nearly equal values. Reconnect and verify the quiet-room readings return.
- [ ] 4.5 Check the overflow and unexpected-channel counters after at least 30 seconds of monitoring; verify both remain zero (log them once at the end of a window, or read them through the debugger if preferred).
- [ ] 4.6 Review the overflow path, which cannot be triggered on working hardware without a deliberately slow consumer: verify from the code that the callback only increments a counter and that `flush_pool` is set.

## Workflow follow-up

- Archive the change after the board checks in group 4 pass.
- Start the clap detection change once this one is verified, using the recorded quiet-room RMS as the starting point for its threshold.
