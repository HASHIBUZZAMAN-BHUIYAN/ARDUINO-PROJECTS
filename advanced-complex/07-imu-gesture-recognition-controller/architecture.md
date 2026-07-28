# Architecture — IMU Gesture Recognition Controller

## System Diagram

```
 +----------------+
 |  MPU6050 (I2C) |
 +--------+-------+
          | (sampled in Timer1 ISR, independent of main loop)
          v
 +------------------------+
 |  Ring buffer (accel mag)|
 +-----------+-------------+
             v
 +------------------------------+
 |  Gesture-window detector      |
 |  (threshold crossing -> settle)|
 +---------------+---------------+
                 v
 +-------------------------------+
 |  Feature extraction             |
 |  (peak, axis, duration, ZCR)    |
 +---------------+-----------------+
                 v
 +-------------------------------+      +------------------+
 |  Nearest-neighbor classifier    |<---->|  EEPROM templates|
 +---------------+-----------------+      +------------------+
                 v
 +-------------------------------+
 |  Action dispatch (servos/relay) |
 +---------------------------------+
```

## Data Flow

1. **Sample** — a Timer1 interrupt reads the MPU6050 over I2C at a fixed rate and pushes the acceleration magnitude into a ring buffer, decoupled from the main loop's pace (concurrency).
2. **Detect window** — the main loop watches the ring buffer for a magnitude crossing above a motion threshold, marking the start of a gesture window, and closes the window once magnitude settles back below threshold for a short debounce period.
3. **Extract features** — `extractFeatures()` reduces the windowed samples to a small vector: peak magnitude, dominant axis, window duration, and zero-crossing count.
4. **Classify** — `classify()` computes a distance from this feature vector to every stored template and returns the nearest one if within a confidence radius, else "unrecognized."
5. **Calibrate (offline path)** — during `CAL` mode, the same feature-extraction path is used, but the result is written to EEPROM as a new named template instead of being classified.
6. **Act** — a recognized gesture name is mapped to an action: pan/tilt servo moves or a light-relay toggle.

## Component Roles

- **MPU6050** — sole sensing input, the basis for every feature.
- **Timer1 ISR** — guarantees consistent sample timing regardless of what the classifier/action code is doing.
- **Feature extractor** — the hand-designed "model input" that keeps the classifier lightweight enough to run on an 8-bit Nano.
- **Nearest-neighbor classifier + EEPROM templates** — the on-device "ML" decision layer.
- **Servos + relay** — the physical actions gestures trigger.
