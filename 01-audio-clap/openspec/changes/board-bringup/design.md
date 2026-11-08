# Design

## Context

The project is an ESP-IDF v6.1-dev skeleton targeting the ESP32-S3, with an empty `app_main` in `main/01-audio-clap.cpp`. See proposal.md for why this change comes first.

Relevant values in the tracked `sdkconfig`:

| Setting | Current value | Status |
|---|---|---|
| Flash size | `CONFIG_ESPTOOLPY_FLASHSIZE_2MB` | Wrong for this board |
| Primary console | `CONFIG_ESP_CONSOLE_UART_DEFAULT` | Wrong for this board |
| Secondary console | `CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG` | Becomes unnecessary |
| Partition table | `CONFIG_PARTITION_TABLE_SINGLE_APP` | Kept |
| Tick rate | `CONFIG_FREERTOS_HZ=100` | Kept |

The project has no `sdkconfig.defaults`, and nothing in the project yet records why these values differ from the IDF defaults.

The board facts used here (4 MB of flash and a WS2812 on GPIO21) come from the board's published specifications, not from anything in this repository. This change's first flash confirms them.

## Goals / Non-Goals

**Goals:**

- Make the board settings reproducible from a small, reviewable file.
- Prove flashing, logging, flash detection, and LED control on real hardware.
- Leave a status LED module that the clap detector can reuse without changes.

**Non-Goals:**

- A general board abstraction layer. The project targets one board.
- LED animation or effects beyond setting one color at a time.
- Automated tests. This change's behavior depends on the physical board, so it is verified by hand.

## Decisions

### Keep board settings in `sdkconfig.defaults`

`sdkconfig.defaults` records only the settings the project cares about: the target, the flash size, and the primary console. `sdkconfig` is then regenerated from it. Editing through `menuconfig` alone was rejected because the generated file is about 90 KB, so the intent would be buried in it and lost on any regeneration.

Existing `sdkconfig` values take precedence over `sdkconfig.defaults`, so adding the defaults file has no effect until `sdkconfig` is regenerated. The tasks therefore delete and regenerate it. It stays tracked, as it is today.

### Keep the default 1 MB app partition

`partitions_singleapp.csv` defines a 1 MB factory app regardless of flash size. Declaring 4 MB therefore doesn't give the firmware more room; it only makes the declared size correct and leaves the rest of the flash unallocated. A custom table was rejected because clap detection should fit comfortably in 1 MB, and nothing yet needs the extra space. The planned detector is DSP only, with no model data to store.

### Report flash sizes from two sources

`esp_flash_get_size` returns the size from the image header, and `esp_flash_get_physical_size` reads the chip's ID. Logging both, and warning when they differ, is what shows that the configuration matches the hardware. Printing only the header value would just repeat the configuration. The physical read is based on the chip ID format, which isn't guaranteed for every flash part, so a mismatch or read failure warns rather than aborts.

### Use `espressif/led_strip` with the RMT backend

This is the component IDF's own `get-started/blink` example uses, at `^3.0.0`, with the same RMT setup and a 10 MHz resolution. Bit-banging the WS2812 protocol was rejected because its timing is tight and interrupts could corrupt it, and writing a custom RMT encoder would duplicate the component. DMA stays off because one LED needs very little data.

### Expose a small `status_led` module

The module lives in `main/status_led.{h,cpp}` with three operations: initialize, set a color, and turn off. `app_main` owns the heartbeat loop, so the clap detector can later flash the LED without inheriting a blink policy. The GPIO and the 16/255 brightness cap are constants inside the module. Brightness stays low because the LED is only an indicator, and a lower current draw also reduces the risk of it coupling into the MAX4466's supply later.

Initialization failures use `ESP_ERROR_CHECK`, matching the IDF examples. A failure here means the firmware or board is misconfigured, and the resulting abort prints the cause on the console.

### Combine the heartbeat into one loop

`app_main` advances the LED color and logs the count once per second in a single loop using `vTaskDelay(pdMS_TO_TICKS(1000))`. One second is well above the 10 ms tick, and the heartbeat doesn't need precise timing. A separate task isn't justified, since this loop exists only until later changes give the firmware real work.

## Risks / Trade-offs

- **[The LED's color order may differ from the component's default]** → The red, green, blue heartbeat makes a swap obvious. If colors are swapped, set the correct format in the module's `led_strip` configuration.
- **[A bad image or console setting can make the USB port stop responding for flashing]** → Enter download mode by holding BOOT while tapping RESET, then reflash.
- **[Output can be missed before a monitor attaches, because the USB console has no buffering host]** → Check the boot report using `idf.py monitor`, which resets the chip on attach so that boot output is visible.
- **[A log call could stall when no host is reading the USB console]** → The heartbeat has no timing requirement, so this is harmless here, and the spec's no-monitor scenario confirms the LED keeps cycling. Later audio code must not log from its capture path.
- **[The first build needs network access to fetch `led_strip`]** → Commit `dependencies.lock` to pin the resolved version. `managed_components` is already ignored.
- **[The 4 MB and GPIO21 facts might not match this particular board]** → The boot report and heartbeat test both directly, on the first flash.

## Migration Plan

Regenerating `sdkconfig` may pick up default values that differ from the ones in the current file. The tasks include checking the diff for changes beyond the intended settings. Rollback is a matter of restoring the tracked `sdkconfig` and removing the new files.
