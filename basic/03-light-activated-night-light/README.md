# Light-Activated Night Light

An LDR (light-dependent resistor) reads ambient brightness and automatically turns an LED "lamp" on at dusk and off at dawn, with a bit of hysteresis so it doesn't flicker at the threshold.

## Difficulty & Board

**Tier:** Basic
**Board:** Arduino Nano

Reasoning: this is a compact, low-pin-count sensor + actuator build meant to live tucked in a small enclosure (e.g. a windowsill), which is exactly the kind of project the Nano's small footprint suits — a full-size Uno would be overkill.

## Components

| Part | Qty |
|---|---|
| Arduino Nano | 1 |
| LDR (photoresistor) | 1 |
| 10 kΩ resistor (LDR pull-down) | 1 |
| LED (5mm, warm white or yellow) | 1 |
| 220 Ω resistor | 1 |
| Breadboard | 1 |
| Jumper wires | 5 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| 5V | LDR leg 1 | |
| A0 | LDR leg 2 + 10kΩ resistor (junction) | voltage divider midpoint |
| GND | 10kΩ resistor other leg | |
| D9 | LED anode via 220 Ω resistor | PWM-capable pin, used for a soft fade-on |
| GND | LED cathode | |

## How It Works

The LDR and the 10 kΩ resistor form a voltage divider: as ambient light drops, the LDR's resistance rises, so the voltage at the midpoint (read on `A0`) changes. The sketch reads that analog value every loop and compares it against two thresholds rather than one:

- Below the **dark threshold** → turn the LED on.
- Above the **light threshold** (set noticeably higher than the dark one) → turn the LED off.
- In between → do nothing (hold the previous state).

That gap between the two thresholds is **hysteresis**: it stops the light from rapidly flicking on/off when ambient brightness hovers right at one single cutoff (e.g. clouds passing, someone walking by). When switching on, the LED is faded in smoothly with `analogWrite()` instead of snapping on, since it's on a PWM pin.

## Setup & Flashing

1. Wire the LDR voltage divider and LED as above.
2. Open `src/night_light.ino` in the Arduino IDE.
3. Select **Tools > Board > Arduino Nano** (and the correct processor — old bootloader if your clone needs it) and the correct COM port.
4. Upload — no external libraries required.
5. Open the Serial Monitor at 9600 baud, then cover/uncover the LDR with your hand to watch the raw reading and confirm the thresholds suit your room's lighting; adjust `DARK_THRESHOLD` / `LIGHT_THRESHOLD` if needed.

## Extensions

- Replace the single LED with a small strip via a MOSFET to light a larger area.
- Add an RTC module so the light only activates during actual nighttime hours, ignoring daytime shadows.
- Log brightness readings over time to characterize the room's day/night cycle.
