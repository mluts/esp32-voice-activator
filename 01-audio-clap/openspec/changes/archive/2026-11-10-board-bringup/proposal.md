# Proposal

## Why

The project's configuration does not match the ESP32-S3-Zero: it declares 2 MB of flash on a 4 MB board and routes the primary console to UART0, which has no USB bridge on this board. `app_main` is also empty, so nothing yet shows that flashing, logging, and the onboard LED work. Those need proving before ADC capture and clap detection are built on top of them.

## What Changes

- Declare the board's 4 MB of flash in the project configuration.
- Make USB-Serial-JTAG the primary console, so logs arrive over the board's native USB port.
- Record these settings in a tracked `sdkconfig.defaults`, with the generated `sdkconfig` regenerated from it.
- Log the board's identity at boot: chip details, and configured versus physically detected flash size.
- Add a small status LED module that drives the onboard WS2812 on GPIO21, for use by the later clap detector.
- Replace the empty `app_main` with a heartbeat that cycles the LED through red, green, and blue and logs a running count.

Out of scope:

- ADC capture, audio processing, and clap detection, which are separate follow-up changes.
- Enabling the board's 2 MB of PSRAM.
- Changing the partition table. The default single-app layout gives the firmware 1 MB, which should be ample for clap detection.
- Changing compiler optimization, CPU frequency, or the FreeRTOS tick rate.

## Capabilities

### New Capabilities

- `board-config`: The firmware's configuration matches the ESP32-S3-Zero, and it reports its flash and console setup at boot.
- `status-led`: The firmware can drive the onboard WS2812 as a visible status indicator.

### Modified Capabilities

None. The project has no existing specs.

## Impact

- **Code**: `main/01-audio-clap.cpp` and `main/CMakeLists.txt` change; a status LED module is added under `main/`.
- **Configuration**: `sdkconfig.defaults` is added and the tracked `sdkconfig` is regenerated.
- **Dependencies**: Adds the `espressif/led_strip` managed component through a new `main/idf_component.yml`, plus a generated `dependencies.lock`. The first build needs network access to fetch it.
- **Workflow**: Flashing and monitoring use the board's native USB port. Verifying this change requires the physical board.
