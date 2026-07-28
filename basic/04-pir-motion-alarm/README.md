# PIR Motion Alarm

A passive-infrared (PIR) motion sensor triggers an audible buzzer alarm whenever it detects movement, with a status LED that mirrors the sensor's raw state.

## Difficulty & Board

**Tier:** Basic
**Board:** Arduino Uno

Reasoning: single digital sensor input plus two digital outputs — the most standard possible pin layout, so the default Uno was used with no particular driver.

## Components

| Part | Qty |
|---|---|
| Arduino Uno | 1 |
| HC-SR501 PIR motion sensor module | 1 |
| Piezo buzzer | 1 |
| LED | 1 |
| 220 Ω resistor | 1 |
| Breadboard | 1 |
| Jumper wires | 5 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| 5V | PIR VCC | |
| GND | PIR GND | |
| D2 | PIR OUT | digital HIGH while motion detected |
| D7 | Buzzer + | |
| D13 | LED anode via 220 Ω resistor | mirrors PIR state (built-in LED pin doubles as status) |
| GND | Buzzer -, LED cathode | shared ground rail |

## How It Works

The HC-SR501 module does all the infrared sensing and thresholding internally — it simply outputs a clean digital HIGH for a few seconds whenever it detects a moving heat source, then drops back to LOW. The sketch just watches that one pin:

- On the rising edge (LOW → HIGH), it prints a timest/motion event to Serial and starts sounding the buzzer.
- While the pin stays HIGH, the buzzer continues (using non-blocking `tone()`/`noTone()` rather than `delay()`, so the sketch stays responsive).
- On the falling edge, the buzzer stops.

Most PIR modules have two on-board trimmer potentiometers for **sensitivity** (detection range) and **time delay** (how long OUT stays HIGH after a trigger) — this project also covers reading those datasheet-level analog behaviors as a fixed digital signal, which is the core beginner concept: treating a "smart" sensor module's processed output as a simple boolean.

## Setup & Flashing

1. Wire the PIR module, buzzer, and LED as above.
2. Open `src/pir_motion_alarm.ino` in the Arduino IDE.
3. Select **Tools > Board > Arduino Uno** and the correct COM port.
4. Upload — no external libraries required.
5. **Important:** let the PIR sensor sit undisturbed for 30-60 seconds after power-up; it needs a warm-up/calibration period before readings stabilize. Then wave a hand in front of it to test.

## Extensions

- Add a keypad-based arm/disarm code so the alarm only sounds when armed (see the intermediate two-zone alarm project for a fuller version of this).
- Send an SMS/notification via an add-on GSM or WiFi module when motion is detected while away.
- Log trigger timestamps to an SD card for a simple activity log.
