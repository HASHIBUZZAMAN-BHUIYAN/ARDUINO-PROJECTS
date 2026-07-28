# Audio Spectrum LED Matrix Visualizer

An Uno samples a microphone at a fixed interrupt-driven rate, computes a 16-band FFT spectrum, drives an 8x8 MAX7219 LED matrix as a live bar-graph visualizer, and logs timestamped "loud event" entries to SD whenever sustained loudness crosses a configurable threshold.

## Board

**Arduino Uno** — a microphone module, one SPI LED matrix, an RTC, an SD module, and a sensitivity pot is a modest pin count; complexity here is in the signal-processing/concurrency split, not I/O volume.

## Components

| Part | Qty |
|---|---|
| Arduino Uno | 1 |
| Electret microphone amplifier module (e.g. MAX9814/MAX4466) | 1 |
| MAX7219 8x8 LED matrix module | 1 |
| DS3231 RTC module (I2C) | 1 |
| microSD card module (SPI) + microSD card | 1 |
| 10 kΩ potentiometer (sensitivity) | 1 |
| Jumper wires | ~15 |

## Architecture

A Timer1 CTC interrupt samples the microphone's analog output at a fixed rate into a 128-sample ring buffer, decoupled from the main loop (concurrency). Once a buffer fills, the main loop runs an FFT, buckets the magnitude spectrum into 16 bands, and drives the MAX7219 matrix as a bar graph (one column per band) with peak-hold. In parallel, the same buffer's RMS loudness is compared against a potentiometer-set threshold; loudness sustained above threshold for more than 1 second is logged as a timestamped "loud event" to SD. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| A0 | Microphone module AOUT | sampled in Timer1 ISR |
| A1 | Sensitivity potentiometer wiper | |
| D10 (CS), D11 (DIN), D13 (CLK) | MAX7219 matrix (SPI) | |
| A4/A5 | DS3231 RTC (I2C) | |
| D4 (SS), D11/D12/D13 | SD module | shares SPI bus with the matrix; SD uses its own CS on D4 |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Setup & Deployment

1. Wire the microphone, LED matrix, RTC, SD module, and sensitivity pot as above; note the LED matrix and SD module share the hardware SPI bus but use distinct CS pins (D10 and D4).
2. Install `arduinoFFT`, `LedControl` (for the MAX7219), `RTClib`, and `SD` (see `libraries.txt`).
3. Open `src/audio_visualizer.ino`, upload to the Uno.
4. Speak/play music near the microphone and confirm the LED matrix bar graph responds live.
5. Turn the sensitivity pot and confirm the loud-event threshold (visible as a Serial-printed value) changes accordingly.
6. Sustain a loud sound for over a second and confirm a row is appended to `NOISELOG.CSV` on the SD card.

## Known Limitations & Path to Production

- Single electret mic with no calibration to an absolute dB scale — loudness is relative, not an SPL measurement suitable for compliance/regulatory logging.
- 16-band FFT resolution is coarse; a production audio analyzer would use a larger FFT window and/or a dedicated codec for better frequency resolution.
- No enclosure/windscreen guidance for outdoor or noisy-environment deployment, which would significantly affect real-world accuracy.

## Extension Ideas

- Add a second color channel (RGB matrix) to show loudness intensity via color in addition to bar height.
- Push loud-event alerts over a simple serial Bluetooth module for a phone notification.
- Add a rolling daily loudness-histogram summary computed on-device, similar to the buoy/weather-station patterns elsewhere in this tier.
