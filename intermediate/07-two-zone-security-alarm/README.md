# Two-Zone Security Alarm

A keypad arms/disarms a house alarm covering two independent zones (e.g. front door reed switch + a PIR motion sensor in a hallway), with a short entry delay before sounding the siren so you have time to disarm after coming in.

## Difficulty & Board

**Tier:** Intermediate
**Board:** Arduino Uno

Reasoning: a 4x4 keypad (8 pins) plus two zone sensors and a siren output is a moderate pin count that fits comfortably on an Uno without needing the Mega's extra headroom.

## Components

| Part | Qty |
|---|---|
| Arduino Uno | 1 |
| 4x4 matrix keypad | 1 |
| Magnetic reed switch (door/window sensor) | 1 |
| HC-SR501 PIR motion sensor | 1 |
| Piezo buzzer/siren | 1 |
| Red LED (armed indicator) | 1 |
| Green LED (disarmed indicator) | 1 |
| 220 Ω resistor | 2 |
| 10 kΩ resistor (reed switch pull-down) | 1 |
| Breadboard | 1 |
| Jumper wires | ~16 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| D2-D5 | Keypad rows 1-4 | |
| D6-D9 | Keypad columns 1-4 | |
| A0 | Reed switch (Zone 1: door) | with 10kΩ pull-down to GND, other side of switch to 5V |
| A1 | PIR OUT (Zone 2: motion) | |
| D10 | Siren/buzzer + | |
| D11 | Red LED via 220 Ω | armed |
| D12 | Green LED via 220 Ω | disarmed |

## How It Works

The system is a small state machine with three states: **DISARMED**, **ARMED**, and **TRIGGERED**.

- In **DISARMED**, both zone sensors are ignored; the green LED is lit.
- Typing the correct 4-digit code (terminated with `#`) while disarmed switches to **ARMED** (red LED lit) after a short exit delay, giving you time to leave and shut the door without immediately triggering Zone 1.
- While **ARMED**, either zone going active starts an **entry delay** countdown (printed to Serial as a courtesy) rather than sounding the siren instantly — if the correct code is entered before the countdown reaches zero, the system returns to DISARMED silently. If the countdown expires first, or if a zone trips again while the entry delay itself is still counting from a *different* prior trip, the system moves to **TRIGGERED** and the siren sounds continuously until the correct code is entered.
- The correct code always disarms from any state, including **TRIGGERED**.

This project's core new concept versus the basic-tier PIR alarm is the explicit state machine plus keypad-based numeric input — most real alarm panels are built on exactly this pattern (arm/disarm states + entry/exit delays + a code).

## Setup & Flashing

1. Wire the keypad, both zone sensors, siren, and LEDs as above.
2. Install the **Keypad** library (by Mark Stanley / Alexander Brevig) via Library Manager.
3. Open `src/two_zone_alarm.ino`; change `const char *CODE = "1234";` to your own 4-digit code.
4. Select **Tools > Board > Arduino Uno** and the correct COM port, then upload.
5. Test: arm the system, wait for the exit delay, trip a zone, and confirm the entry delay gives you time to disarm before the siren sounds. Then test that letting the countdown expire does trigger the siren, and that the code silences it.

## Extensions

- Add a third zone (e.g. a second reed switch on a back door) — the state machine already generalizes to N zones.
- Log every arm/disarm/trigger event with a timestamp to an SD card (see the advanced-tier RFID + keypad access logger for the SD + RTC pattern).
- Add a wireless panic button using the advanced-tier's nRF24L01 dual-board pattern.
