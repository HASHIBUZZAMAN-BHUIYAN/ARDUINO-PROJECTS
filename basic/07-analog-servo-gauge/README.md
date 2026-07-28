# Analog Servo Gauge

Turning a potentiometer sweeps a servo-mounted pointer needle across a printed dial, like an analog volume knob driving a physical VU-meter needle.

## Difficulty & Board

**Tier:** Basic
**Board:** Arduino Nano

Reasoning: only one analog input and one PWM output are needed, and this is typically built into a small desktop dial enclosure — the Nano's small footprint fits that better than a full-size Uno.

## Components

| Part | Qty |
|---|---|
| Arduino Nano | 1 |
| SG90 micro servo | 1 |
| 10 kΩ potentiometer | 1 |
| Breadboard | 1 |
| Jumper wires | 5 |
| Paper/cardstock dial + pointer (craft materials, not electronic) | 1 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| 5V | Potentiometer outer pin 1, servo VCC (red) | |
| GND | Potentiometer outer pin 2, servo GND (brown/black) | shared ground |
| A0 | Potentiometer wiper (middle pin) | reads knob position |
| D9 | Servo signal (orange/yellow) | PWM-capable pin required by `Servo` library |

## How It Works

The potentiometer's wiper voltage (0-5V) is read as a 0-1023 value on `A0`. `map()` rescales that range down to the servo's usable angle range (roughly 0-180 degrees, though many hobby servos bind up at the extremes, so this sketch trims to 10-170 to protect the gears). The mapped angle is sent to the servo every loop via the standard `Servo` library, so the needle position always tracks the knob position in real time.

A small amount of smoothing (only updating the servo if the target angle has changed by more than 1 degree) prevents the servo from buzzing/jittering on tiny ADC noise around a steady knob position — this is the core lesson: raw ADC readings are noisy even when nothing is moving, and physical actuators need a bit of deadband to sit still.

## Setup & Flashing

1. Wire the potentiometer and servo as above.
2. Open `src/servo_gauge.ino` in the Arduino IDE.
3. Select **Tools > Board > Arduino Nano** (correct processor/bootloader for your clone) and the correct COM port.
4. Install the **Servo** library if not already present (it ships built-in with the Arduino IDE's AVR core, so normally no action is needed).
5. Upload, then attach a paper pointer to the servo horn and turn the knob to watch the needle follow.

## Extensions

- Replace the potentiometer with a sensor (light, temperature, distance) to build a real analog gauge instrument.
- Add a second servo and a printed dual-needle dial (e.g. min/max temperature).
- Add tick marks and calibrate `map()` so the needle visually matches real printed units (e.g. 0-100%).
