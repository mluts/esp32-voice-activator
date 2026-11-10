# Spec Delta

## Purpose

Captures audio from MAX4466 microphones as a steady stream of fixed-size sample frames, so that later detection code has a reliable signal to work on, and makes the microphone's wiring and signal level visible from a connected computer.

## ADDED Requirements

### Requirement: Microphones are configured by GPIO number
The firmware SHALL accept the set of microphone input pins as GPIO numbers, with a single microphone on GPIO1 as the default, and SHALL accept only pins that are ADC1 inputs on the ESP32-S3 (GPIO1 through GPIO10).

#### Scenario: Default microphone pin
- **WHEN** the firmware is built without naming a microphone pin
- **THEN** it captures from GPIO1

#### Scenario: Pin is not an ADC1 input
- **WHEN** a configured microphone pin is an ADC2 input or not an ADC input at all
- **THEN** the firmware stops during startup and the console shows which pin was rejected and why

### Requirement: Audio is captured at a fixed rate in fixed-size frames
The firmware SHALL sample every configured microphone at 16 kHz and deliver the samples as frames of 256 samples per microphone, so that each frame covers 16 ms.

#### Scenario: Frames arrive at the expected cadence
- **WHEN** capture is running with one microphone
- **THEN** about 62 frames of 256 samples are delivered per second

#### Scenario: Several microphones share the frame
- **WHEN** capture is running with two microphones
- **THEN** each frame carries 256 samples for each microphone, and no microphone's samples appear in another's

### Requirement: Samples are signed and centered on silence
Each delivered sample SHALL be a signed 16-bit value with the microphone's DC bias removed, so that silence reads near zero and sound swings positive and negative.

#### Scenario: Quiet room
- **WHEN** the room is quiet and the microphone has been running for at least one second
- **THEN** the samples in a frame average near zero, with both positive and negative values present

#### Scenario: Microphones have different bias points
- **WHEN** two microphones idle at different output voltages
- **THEN** each microphone's samples are centered near zero independently

### Requirement: Capture tolerates a slow consumer
The firmware SHALL keep sampling when frames are not consumed promptly, SHALL retain at least 64 ms of audio before discarding the oldest frames, and after a stall SHALL deliver current audio rather than stale frames.

#### Scenario: Consumer is briefly late
- **WHEN** the consumer stops reading for up to 64 ms and then resumes
- **THEN** it receives the frames captured during the pause with no sample loss

#### Scenario: Consumer stalls for longer
- **WHEN** the consumer stops reading for more than 64 ms and then resumes
- **THEN** the oldest frames are discarded and the next frame read reflects recent audio

### Requirement: Signal levels are reported on the console
The firmware SHALL log, about once per second and for each microphone, the minimum and maximum sample, the RMS level, and the raw bias point observed over that second, so that wiring, room level, and loud events can be checked from a serial monitor.

#### Scenario: Microphone is wired
- **WHEN** a MAX4466 is powered and connected to the configured pin
- **THEN** the logged bias is roughly mid-scale and the RMS is small but not zero in a quiet room

#### Scenario: Microphone is not connected
- **WHEN** the configured pin is left floating or tied to ground
- **THEN** the logged values are visibly different from a connected microphone: the bias is far from mid-scale or the minimum and maximum are nearly equal

#### Scenario: Clap registers
- **WHEN** a hand clap occurs near the microphone during a one-second window
- **THEN** that window's logged minimum and maximum are several times larger in magnitude than the quiet-room values

#### Scenario: Gain is too high
- **WHEN** a clap drives the microphone output past the ADC's input range
- **THEN** the logged minimum or maximum reaches the end of the sample range, showing that the gain pot must be turned down

### Requirement: Capture does not disturb the heartbeat
While capture is running, the LED and log heartbeat SHALL continue at about once per second.

#### Scenario: Heartbeat runs alongside capture
- **WHEN** a serial monitor stays attached for at least 10 seconds with capture running
- **THEN** heartbeat lines keep appearing about once per second with an increasing count, and the LED keeps cycling red, green, blue
