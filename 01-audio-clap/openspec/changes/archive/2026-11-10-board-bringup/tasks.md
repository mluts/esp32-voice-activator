# Tasks

## 1. Board configuration

- [x] 1.1 Add `sdkconfig.defaults` setting `CONFIG_IDF_TARGET="esp32s3"`, `CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y`, and `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`; verify the file contains only these board settings.
- [x] 1.2 Delete `sdkconfig` and regenerate it with `idf.py reconfigure`; verify it targets `esp32s3`, sets `CONFIG_ESPTOOLPY_FLASHSIZE="4MB"` and `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`, and that `llm-git diff -- sdkconfig` shows no unexplained changes (the tracked `sdkconfig.old` may also change).

## 2. Status LED module

- [x] 2.1 Add `main/idf_component.yml` depending on `espressif/led_strip` at `^3.0.0`; verify `idf.py reconfigure` fetches the component and generates `dependencies.lock`.
- [x] 2.2 Add `main/status_led.h` and `main/status_led.cpp` with initialize, set color, and off operations, using RMT at 10 MHz on GPIO21, a 16/255 brightness cap, and the LED off after initialization; list the source in `main/CMakeLists.txt` and verify `idf.py build` succeeds.

## 3. Application

- [x] 3.1 Add a boot report to `app_main` logging chip model, revision, core count, and configured and physical flash sizes, with a warning on a size mismatch or a failed read; declare `spi_flash` in `main/CMakeLists.txt` if the build requires it, and verify `idf.py build` succeeds.
- [x] 3.2 Add the heartbeat loop to `app_main`, advancing red, green, blue and logging an increasing count once per second; verify `idf.py build` succeeds and report the app size and free space in the 1 MB partition using `idf.py size`.

## 4. Verification on the board

These tasks need the physical ESP32-S3-Zero connected by USB. Tasks 4.1 to 4.4 depend on the firmware from group 3.

- [x] 4.1 Run `idf.py -p <port> flash monitor` over the native USB port; verify flashing succeeds and that boot output and application logs appear with no UART adapter.
- [x] 4.2 Check the boot report; verify it shows the chip details, 4 MB for both configured and physical flash, and no flash size warning.
- [x] 4.3 Watch the monitor and LED for at least 10 seconds; verify the count rises by one about once per second and the LED shows red, then green, then blue. If colors are swapped, correct the color format in `main/status_led.cpp` and repeat this check.
- [x] 4.4 Close the monitor and power the board from USB; verify the LED keeps cycling for at least 30 seconds.
- [x] 4.5 Temporarily set the flash size to 2 MB with `idf.py menuconfig`, then flash and monitor; verify the mismatch warning states both sizes and the heartbeat keeps running. Then restore `sdkconfig` and reflash, and verify the warning is gone.
- [x] 4.6 Review the failed-read branch of the boot report, which can't be triggered on working hardware; verify from the code that it logs a warning and continues to the heartbeat.

## Workflow follow-up

- Archive the change after the board checks in group 4 pass.
- Start the `adc-audio-capture` change once this one is verified.
