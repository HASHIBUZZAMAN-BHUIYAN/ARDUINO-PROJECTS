# Ultrasonic Parking Sensor

A single HC-SR04 ultrasonic sensor measures distance to an obstacle (like a car bumper approaching a garage wall) and speeds up a buzzer beep as the object gets closer, going solid when it's too close.

## Difficulty & Board

**Tier:** Basic
**Board:** Arduino Uno

Reasoning: HC-SR04 needs exactly two digital pins (trigger/echo) plus power — no ambiguity here, any board works, so the default Uno is used.

## Components

| Part | Qty |
|---|---|
| Arduino Uno | 1 |
| HC-SR04 ultrasonic distance sensor | 1 |
| Piezo buzzer (active or passive) | 1 |
| Breadboard | 1 |
| Jumper wires | 6 |

## Wiring

| Arduino Pin | Component Pin | Notes |
|---|---|---|
| 5V | HC-SR04 VCC | |
| GND | HC-SR04 GND | |
| D9 | HC-SR04 TRIG | sends the ultrasonic pulse |
| D10 | HC-SR04 ECHO | reads the pulse return time |
| D6 | Buzzer + | |
| GND | Buzzer - | |

## How It Works

1. The sketch pulses `TRIG` high for 10 microseconds, which fires an ultrasonic burst.
2. `pulseIn()` times how long `ECHO` stays high — that duration is proportional to the round-trip time of the sound wave.
3. Distance in centimeters is calculated from the classic formula `distance = duration * 0.0343 / 2` (speed of sound in air ≈ 343 m/s, divided by 2 because the pulse travels to the object and back).
4. The distance is mapped to a buzzer beep interval: far away = slow occasional beeps, closer = faster beeps, and inside a "danger zone" (< 10 cm) the buzzer goes continuous.

This single-sensor project teaches timed pulse measurement (`pulseIn`) and mapping a continuous analog-like reading onto a simple feedback pattern.

## Setup & Flashing

1. Wire the HC-SR04 and buzzer as shown above.
2. Open `src/parking_sensor.ino` in the Arduino IDE.
3. Select **Tools > Board > Arduino Uno** and the correct COM port.
4. Upload — no external libraries required, `pulseIn()` is a core Arduino function.
5. Open the Serial Monitor at 9600 baud to see live distance readings in centimeters while you test with your hand.

## Extensions

- Add an LED bar graph (or a strip of 5 LEDs) to show distance visually, not just audibly.
- Average several readings together to smooth out HC-SR04 noise/outliers.
- Add a second sensor and average/compare both for a wider detection cone.
