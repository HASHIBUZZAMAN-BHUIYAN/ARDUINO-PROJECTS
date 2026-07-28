# IMU Gesture Recognition Controller

A Nano samples an IMU at a fixed interrupt-driven rate, extracts motion features over detected gesture windows, and runs an on-device nearest-neighbor classifier against calibration-recorded templates to recognize hand gestures and drive a pan-tilt mount and a light relay.

## Board

**Arduino Nano** — a single I2C IMU plus 2 servos and a relay is a small pin count; the Nano's compact form factor also suits this project's implied "wearable/handheld controller" use case.

## Components

| Part | Qty |
|---|---|
| Arduino Nano | 1 |
| MPU6050 IMU (I2C) | 1 |
| Micro servo (pan) | 1 |
| Micro servo (tilt) | 1 |
| Relay module (light) | 1 |
| Push button (calibration mode trigger) | 1 |
| Jumper wires | ~12 |

## Architecture

A Timer1 CTC interrupt samples the MPU6050's acceleration magnitude at a fixed rate into a ring buffer, independent of whatever the main loop is doing (concurrency). When the magnitude crosses a motion threshold, the main loop opens a "gesture window," accumulates it until motion settles again, then extracts a small feature vector (peak magnitude, dominant axis, duration, zero-crossing count). This feature vector is compared via nearest-neighbor distance against a set of templates recorded during a calibration mode and stored in EEPROM; a matched gesture drives the pan/tilt servos or toggles the light relay. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

| Arduino Nano Pin | Component | Notes |
|---|---|---|
| A4 (SDA) | MPU6050 SDA | |
| A5 (SCL) | MPU6050 SCL | |
| D9 | Pan servo signal | |
| D10 | Tilt servo signal | |
| D6 | Relay IN (light) | |
| D2 | Calibration button | `INPUT_PULLUP`, active LOW |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

No network stack. A serial calibration protocol (9600 baud) is used during setup:

- `CAL START <name>` — begin recording a new gesture template under `<name>`.
- perform the gesture once — the sketch auto-detects the window via the motion threshold and captures its feature vector.
- `CAL SAVE` — commits the captured feature vector to the next free EEPROM template slot (up to 5 templates).
- `CAL LIST` — prints all currently stored template names and their slot index.

## Setup & Deployment

1. Wire the MPU6050, both servos, the relay, and the calibration button as above.
2. Install `MPU6050` (or `Adafruit MPU6050` + `Adafruit Unified Sensor`) and `EEPROM` (see `libraries.txt`).
3. Upload `src/gesture_controller.ino` to the Nano; open Serial Monitor at 9600 baud.
4. Record at least 3 distinct gesture templates (e.g. "swipe-left", "swipe-right", "shake") using the `CAL` protocol above, holding the board as you would in normal use.
5. Exit calibration (send `CAL DONE`) and perform each trained gesture; confirm the Serial Monitor prints the recognized name and the corresponding servo/relay action fires.
6. If recognition is unreliable, re-record templates more consistently (same grip, similar speed) — the nearest-neighbor classifier is only as good as its templates.

## Known Limitations & Path to Production

- Nearest-neighbor on a small hand-picked feature vector is simple and interpretable but far less robust than a trained model (e.g. a TinyML neural net) to variation in gesture speed/amplitude across users.
- Only 5 EEPROM template slots; extending this needs either more EEPROM-efficient feature encoding or external flash.
- No debounce/cooldown tuning exposed at runtime — gesture windows too close together can misfire.

## Extension Ideas

- Add a second calibration template per gesture (recorded at a different speed) and average/pick the nearest of either.
- Replace nearest-neighbor with a tiny trained classifier (e.g. via Edge Impulse) for better generalization.
- Add a low-power sleep mode between gesture windows for battery-powered wearable use.
