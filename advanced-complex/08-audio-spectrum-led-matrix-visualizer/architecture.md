# Architecture — Audio Spectrum LED Matrix Visualizer

## System Diagram

```
 +----------------+
 |  Microphone    |
 |  module (A0)   |
 +--------+-------+
          | (sampled in Timer1 ISR)
          v
 +------------------------+
 |  128-sample ring buffer |
 +-----------+-------------+
             v
 +------------------------------+
 |  arduinoFFT (16-band bucket)  |
 +---------------+---------------+
                 v
      +----------+-----------+
      v                      v
+--------------+     +------------------+
| MAX7219 bar   |     | RMS loudness     |
| graph display |     | threshold check  |
+--------------+     +--------+---------+
                               v
                      +------------------+
                      | SD loud-event log |
                      | (RTC timestamp)    |
                      +------------------+
```

## Data Flow

1. **Sample** — a Timer1 interrupt reads A0 at a fixed rate (e.g. 4kHz) into a ring buffer, independent of the main loop's display/logging work.
2. **Transform** — once 128 samples accumulate, `arduinoFFT` computes the magnitude spectrum.
3. **Bucket** — `bucketToBands()` groups FFT bins into 16 logical bands matched to the matrix's 8 columns (or 16 sub-columns via a 2-frame cycle) with peak-hold decay.
4. **Display** — `drawBars()` pushes the per-band bar heights to the MAX7219 over SPI.
5. **Loudness check** — `computeRMS()` on the same buffer feeds a threshold comparison against the potentiometer-set sensitivity; sustained excess starts a timer.
6. **Log** — once loudness has been sustained above threshold for over 1 second, `logEvent()` appends an RTC-timestamped row to SD.

## Component Roles

- **Microphone module** — sole audio input, pre-amplified to a usable analog range.
- **Timer1 ISR** — fixed-rate sampling independent of variable-length display/SD work in the main loop (the concurrency trait).
- **arduinoFFT** — turns the time-domain buffer into a spectrum on-device.
- **MAX7219 matrix** — real-time visual feedback of the spectrum.
- **Potentiometer** — a physical, no-reflash-needed sensitivity control.
- **RTC + SD** — durable record of loud events for later review.
