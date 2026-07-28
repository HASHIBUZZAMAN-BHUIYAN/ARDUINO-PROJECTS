# Architecture — Vibration FFT Predictive Maintenance Monitor

## System Diagram

```
 +----------------+
 |  ADXL345       |
 |  accelerometer |
 +--------+-------+
          | (I2C, read in Timer1 ISR)
          v
 +------------------------+
 |  256-sample ring buffer|
 +-----------+-------------+
             v
 +------------------------------+
 |  arduinoFFT (every 2s window)|
 +---------------+---------------+
                 v
 +-------------------------------+
 |  Dominant bin + RMS extraction |
 +---------------+-----------------+
                 v
 +-------------------------------+       +----------------+
 |  Baseline comparison /         |------>|  SD (RTC time- |
 |  anomaly counter                |       |  stamped log)  |
 +---------------+-----------------+       +----------------+
                 v
 +-------------------------------+
 |  Relay (isolate) + buzzer       |
 +---------------------------------+
```

## Data Flow

1. **Sample** — a Timer1 CTC interrupt fires at a fixed rate (e.g. 800 Hz) and pushes one accelerometer magnitude sample into a ring buffer.
2. **Window** — once 256 samples have accumulated, the main loop copies them into an FFT input array.
3. **Transform** — `arduinoFFT` computes the magnitude spectrum; `findDominantBin()` and `computeRMS()` reduce it to two scalars.
4. **Compare** — during the first ~60 windows after boot, these scalars are averaged into a baseline; afterward, each window's scalars are compared against the baseline with a tolerance band.
5. **Decide** — an anomaly counter increments on out-of-band windows and resets on in-band ones; crossing a consecutive-anomaly threshold trips the fault state.
6. **Act** — on fault, the relay is de-energized (isolating the monitored machine) and the buzzer sounds; every window's summary (timestamp, dominant frequency, RMS, fault flag) is appended to SD regardless of fault state.

## Component Roles

- **ADXL345** — the sole sensing input; sampled at high, fixed rate for legitimate spectral analysis.
- **Timer1 ISR** — guarantees uniform sample spacing, which FFT frequency accuracy depends on (jitter from a variable-rate main loop would smear the spectrum).
- **arduinoFFT** — converts the time-domain buffer into a frequency-domain spectrum on-device, no external compute needed.
- **Baseline/anomaly logic** — the decision layer that turns a spectrum into a pass/fail judgement.
- **Relay** — the closed-loop actuator that removes power from the monitored machine on a confirmed fault.
- **SD + RTC** — durable, timestamped historical record for trend analysis after the fact.
