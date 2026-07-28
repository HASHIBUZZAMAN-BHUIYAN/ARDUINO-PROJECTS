# Wearable Step Counter & Posture Coach

A wrist or belt-worn MPU6050 accelerometer/gyroscope counts steps from the natural bounce of walking, and separately watches for a sustained forward-slouch angle, buzzing a small vibration motor as a posture reminder.

## Difficulty & Board

**Tier:** Intermediate
**Board:** Arduino Nano

Reasoning: an I2C sensor plus a single vibration-motor output is a very small pin count, and the whole point of a wearable is to be physically small and battery-friendly — the Nano is the obvious pick over any larger board here.

## Components

| Part | Qty |
|---|---|
| Arduino Nano | 1 |
| MPU6050 accelerometer/gyroscope module (I2C) | 1 |
| Small vibration motor (coin/pancake type) | 1 |
| N-channel MOSFET or NPN transistor (motor driver) | 1 |
| 1N4001 flyback diode | 1 |
| 220 Ω resistor (transistor base) | 1 |
| 3.7V LiPo battery + charger module (for a real wearable build) | 1 |
| Small enclosure/wristband mount | 1 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| 5V | MPU6050 VCC | most breakout boards include a 3.3V regulator and accept 5V; check yours |
| GND | MPU6050 GND, motor driver emitter/source | shared ground |
| A4 (SDA) | MPU6050 SDA | |
| A5 (SCL) | MPU6050 SCL | |
| D9 | Transistor/MOSFET base/gate via 220 Ω | drives the vibration motor |

## How It Works

**Step counting:** walking produces a repeating pattern in the accelerometer's total acceleration magnitude (`sqrt(ax² + ay² + az²)`) — it dips below 1g during the airborne part of each step and spikes above 1g on footfall. The sketch computes that magnitude every loop, and counts one step each time the signal crosses above a threshold after having first dropped below a lower threshold — this two-threshold crossing (again, hysteresis, like the basic-tier night light) is what prevents small hand jitters from being miscounted as steps.

**Posture check:** separately, the sketch reads the accelerometer's static tilt (when relatively still) to estimate a forward-lean angle using `atan2()` on two of the three axes. If that angle stays beyond a "slouching" threshold continuously for more than a few seconds (rather than just momentarily bending over), the vibration motor gives a few short pulses as a reminder — the sustained-duration check is what distinguishes "you're slouched right now" from "you just bent down to tie your shoe."

## Setup & Flashing

1. Wire the MPU6050 and vibration motor driver stage as above.
2. Install the **Adafruit MPU6050** library via Library Manager, along with its dependencies `Adafruit Unified Sensor` and `Adafruit BusIO`.
3. Open `src/step_counter_posture.ino` in the Arduino IDE.
4. Select **Tools > Board > Arduino Nano** (correct processor/bootloader for your clone) and the correct COM port.
5. Upload, then open the Serial Monitor at 9600 baud. Walk a known number of steps to sanity-check the count, and lean forward and hold to confirm the posture buzz triggers after a few seconds (not instantly).
6. Calibrate `STEP_THRESHOLD_HIGH`/`LOW` and `SLOUCH_ANGLE_DEG` to fit how you actually wear the device (wrist vs. belt mounting changes the useful thresholds significantly).

## Extensions

- Add a small OLED display to show live step count and posture status instead of relying on Serial.
- Add daily step goals with a celebratory buzz pattern on reaching the target.
- Send step/posture summaries to a phone over the Bluetooth pattern from the intermediate smart-lamp project.
