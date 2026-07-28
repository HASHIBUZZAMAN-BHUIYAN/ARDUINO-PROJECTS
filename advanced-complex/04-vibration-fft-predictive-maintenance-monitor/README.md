# Vibration FFT Predictive Maintenance Monitor

An Uno samples an accelerometer at a fixed rate via timer interrupt, runs an on-device FFT to extract a vibration spectrum, flags anomalies against a learned baseline, and closes the loop by cutting power to a monitored machine when the anomaly persists.

## Board

**Arduino Uno** — a single I2C accelerometer, one relay, one buzzer, and an SD+RTC logging pair is a modest pin count; the interesting complexity here is entirely in the signal-processing pipeline, not I/O volume.

## Components

| Part | Qty |
|---|---|
| Arduino Uno | 1 |
| ADXL345 accelerometer (I2C) | 1 |
| DS3231 RTC module (I2C) | 1 |
| microSD card module (SPI) + microSD card | 1 |
| Relay module (machine isolation contactor) | 1 |
| Piezo buzzer | 1 |
| Push button (manual fault reset) | 1 |
| Jumper wires | ~15 |

## Architecture

A Timer1 interrupt samples the ADXL345 at a fixed rate into a 256-sample ring buffer; every 2 seconds the main loop runs an FFT over the most recent window, extracts the dominant frequency bin and RMS amplitude, and compares both against a stored baseline. Sustained deviation beyond threshold for several consecutive windows trips a relay (machine isolation) and a buzzer, and every window's summary is timestamped and appended to SD. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| A4 (SDA) | ADXL345 SDA, DS3231 SDA | Uno's I2C pins |
| A5 (SCL) | ADXL345 SCL, DS3231 SCL | |
| D4 | Relay IN (machine isolation) | |
| D5 | Buzzer + | |
| D2 | Reset button | `INPUT_PULLUP`, active LOW |
| D10 (SS), D11, D12, D13 | SD CS/MOSI/MISO/SCK | hardware SPI |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Setup & Deployment

1. Wire the ADXL345, RTC, SD module, relay, buzzer, and reset button as above.
2. Install `arduinoFFT`, `RTClib`, and `SD` (see `libraries.txt`).
3. Mount the ADXL345 rigidly to the machine housing being monitored — a loose sensor produces meaningless spectra.
4. Open `src/vibration_monitor.ino`, upload to the Uno.
5. Run the machine in known-good condition for at least 2 minutes; the sketch's first 60 windows are treated as a baseline-learning period (printed to Serial as `LEARNING`).
6. After baseline learning, confirm normal operation shows `OK` on Serial and the relay stays energized; simulate an anomaly (tap the sensor sharply, repeatedly) and confirm the relay de-energizes and the buzzer sounds after several consecutive anomalous windows.
7. Press the reset button to re-energize the relay after inspecting the (simulated) fault.

## Known Limitations & Path to Production

- The baseline is learned once at boot from a fixed window count, not adaptively updated — long-term drift (e.g. normal bearing wear) could eventually false-trigger. Production would retrain the baseline periodically during confirmed-healthy runs.
- Single-axis-dominant analysis (uses vector magnitude, not per-axis spectra) — a production system would analyze X/Y/Z independently for directional fault signatures.
- No wireless alerting; an operator must be physically present to see the buzzer/Serial output or check the SD log.

## Extension Ideas

- Add an ESP8266 AT co-processor (see `advanced/07` in this repo) to push anomaly alerts over WiFi.
- Log full per-window spectra (not just dominant bin + RMS) to SD for offline spectral trend analysis.
- Add a second accelerometer axis-pair to distinguish bearing wear from imbalance signatures.
