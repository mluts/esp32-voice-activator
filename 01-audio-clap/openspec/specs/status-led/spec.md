# status-led Specification

## Purpose

Gives the firmware a visible way to signal its state through the ESP32-S3-Zero's onboard RGB LED, without needing a connected computer.

## Requirements

### Requirement: Indicator shows the requested color
The firmware SHALL be able to set the onboard RGB LED to a requested color, with each color channel appearing as requested.

#### Scenario: Primary colors display correctly
- **WHEN** the firmware requests red, then green, then blue
- **THEN** the LED visibly shows red, then green, then blue, with no two colors swapped

### Requirement: Indicator can be turned off
The firmware SHALL be able to turn the onboard RGB LED off, and SHALL start it in the off state during initialization.

#### Scenario: LED is off after initialization
- **WHEN** the indicator has been initialized and no color has been requested
- **THEN** the LED is dark

#### Scenario: LED is turned off after showing a color
- **WHEN** the LED is showing a color and the firmware requests off
- **THEN** the LED goes dark

### Requirement: Indicator shows a running heartbeat
While the firmware is running, the LED SHALL repeatedly cycle through red, green, and blue, holding each color for about one second.

#### Scenario: Heartbeat runs without a computer
- **WHEN** the board is powered by USB with no serial monitor attached
- **THEN** the LED cycles through red, green, and blue for as long as the board stays powered
