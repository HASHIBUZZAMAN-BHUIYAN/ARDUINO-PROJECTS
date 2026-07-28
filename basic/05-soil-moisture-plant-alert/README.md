# Soil Moisture Plant Alert

A capacitive soil moisture probe checks how dry a potted plant's soil is and lights a status LED (plus a short buzzer chirp) when it's time to water.

## Difficulty & Board

**Tier:** Basic
**Board:** Arduino Uno Q

Reasoning: this is meant to be a small, self-contained sensor pod that sits in/near a plant pot, which fits the Uno Q's compact form factor well. Note the Uno Q's GPIO runs at **3.3V logic** (its MCU side is an STM32, unlike the classic 5V Uno) — the capacitive sensor and LED used here are both fine at 3.3V, but double-check datasheet voltage ranges before reusing this wiring on other 3.3V-only boards.

## Components

| Part | Qty |
|---|---|
| Arduino Uno Q | 1 |
| Capacitive soil moisture sensor (v1.2 or similar) | 1 |
| LED | 1 |
| 220 Ω resistor | 1 |
| Piezo buzzer | 1 |
| Breadboard | 1 |
| Jumper wires | 5 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| 3V3 | Soil sensor VCC | capacitive sensors work fine at 3.3V |
| GND | Soil sensor GND | |
| A0 | Soil sensor AOUT | analog reading, higher = drier |
| D5 | LED anode via 220 Ω resistor | "needs water" indicator |
| D6 | Buzzer + | short chirp on state change |
| GND | LED cathode, buzzer - | shared ground |

## How It Works

Capacitive soil sensors measure moisture by sensing changes in capacitance around the probe (unlike cheaper resistive probes, they don't corrode over time since no bare metal contacts the soil directly). The analog output is **inverted** versus what you might expect: a higher reading means *drier* soil, lower means wetter.

The sketch takes several `analogRead()` samples and averages them (soil readings are naturally noisy), compares the average against a dryness threshold, and:

- If dry and the LED wasn't already on, turns the LED on and gives one short buzzer chirp (so it only chirps once per dry event, not continuously).
- If moist again, turns the LED off silently.

This teaches calibrating an analog sensor against real-world thresholds and using a one-shot notification instead of continuous noise.

## Setup & Flashing

1. Wire the sensor, LED, and buzzer as above.
2. Open `src/soil_moisture_alert.ino` in the Arduino IDE.
3. Select **Tools > Board > Arduino Uno Q** and the correct COM port (install the Uno Q board package via Boards Manager first if not already present).
4. Upload — no external libraries required.
5. Open the Serial Monitor at 9600 baud. Insert the probe into dry soil and then into a cup of water to see the raw values at each extreme, and adjust `DRY_THRESHOLD` in the sketch to match your own sensor/soil.

## Extensions

- Add a small water pump + relay to water automatically instead of just alerting (see the intermediate tier for a fuller automated version of this idea).
- Average readings over a whole day and only alert if the daily-average trend is dropping, to ignore momentary spikes.
- Add a second sensor in a different pot and distinguish which plant needs attention via two LEDs.
