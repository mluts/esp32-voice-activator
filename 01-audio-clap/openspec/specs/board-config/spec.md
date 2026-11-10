# board-config Specification

## Purpose

Keeps the firmware's configuration matched to the ESP32-S3-Zero, and makes its flash and console setup visible from a connected computer.

## Requirements

### Requirement: Firmware declares the board's flash size
The firmware image SHALL declare 4 MB of flash.

#### Scenario: Declared size is reported at boot
- **WHEN** the firmware boots on an ESP32-S3-Zero
- **THEN** the boot report shows a configured flash size of 4 MB

### Requirement: Board settings survive configuration regeneration
The project SHALL define its board-specific settings in a tracked source file, so that regenerating the build configuration preserves them.

#### Scenario: Configuration is regenerated from scratch
- **WHEN** the generated configuration is deleted and the project is reconfigured
- **THEN** the regenerated configuration targets the ESP32-S3, declares 4 MB of flash, and uses USB-Serial-JTAG as the primary console

### Requirement: Console uses the native USB port
The firmware SHALL use the board's native USB port as its primary console, without needing an external UART adapter.

#### Scenario: Logs reach a connected computer
- **WHEN** the board is connected by its USB port and a serial monitor is attached
- **THEN** boot output and application logs appear in the monitor

### Requirement: Firmware reports its identity at boot
The firmware SHALL log the chip's model, revision, and core count at boot, along with its configured and physically detected flash sizes.

#### Scenario: Flash sizes match
- **WHEN** the firmware boots and the configured and detected flash sizes are equal
- **THEN** both sizes are logged and no flash size warning appears

#### Scenario: Flash sizes differ
- **WHEN** the firmware boots and the configured and detected flash sizes differ
- **THEN** a warning stating both sizes is logged and the firmware keeps running

#### Scenario: Flash size cannot be detected
- **WHEN** the firmware boots and reading the physical flash size fails
- **THEN** a warning stating the failure is logged and the firmware keeps running

### Requirement: Firmware logs a running heartbeat
The firmware SHALL log a heartbeat with an increasing count about once per second while it is running.

#### Scenario: Monitor shows the firmware is alive
- **WHEN** a serial monitor stays attached for at least 10 seconds after boot
- **THEN** heartbeat lines appear about once per second, with a count that increases by one each time
