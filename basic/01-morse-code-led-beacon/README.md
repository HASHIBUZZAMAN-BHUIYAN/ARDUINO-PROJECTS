# Morse Code LED Beacon

Flashes a fixed text message out on a single LED using Morse code timing, over and over, like a tiny lighthouse.

## Difficulty & Board

**Tier:** Basic
**Board:** Arduino Uno

Reasoning: this is a pure timing exercise on one output pin. An Uno's clock/timer behavior is the most commonly documented for beginners learning `delay()`-based timing, so it's the natural default when no other constraint pushes toward a different board.

## Components

| Part | Qty |
|---|---|
| Arduino Uno | 1 |
| LED (5mm, any color) | 1 |
| 220 Ω resistor | 1 |
| Breadboard | 1 |
| Jumper wires | 2 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| D8 | LED anode (+) via 220 Ω resistor | current-limiting resistor in series |
| GND | LED cathode (-) | direct to ground rail |

See `wiring/wiring.md` for the ASCII diagram.

## How It Works

The message `"SOS EDGECOST"` is stored as a C string. A small translation table maps each letter to its dot/dash pattern. The sketch walks the message character by character:

- A **dot** = LED on for one unit (`DOT_MS`), then off for one unit (inter-symbol gap).
- A **dash** = LED on for three units, then off for one unit.
- A **space between letters** = an extra gap of three units total.
- A **space between words** = a gap of seven units total.

Because Morse timing is entirely ratio-based (a dash is exactly 3 dots, gaps are 1/3/7 units), changing a single `DOT_MS` constant speeds up or slows down the whole message without touching any other logic — this is the "one core concept" the project teaches: relative timing with `delay()`.

## Setup & Flashing

1. Wire the LED and resistor as above.
2. Open `src/morse_code_beacon.ino` in the Arduino IDE.
3. Select **Tools > Board > Arduino Uno** and the correct COM port.
4. Click Upload. No external libraries are required.
5. Watch the LED spell out the message on a loop; open the Serial Monitor at 9600 baud to see the same message printed as dots/dashes in text for cross-checking.

## Extensions

- Add a pushbutton to let the user key in their own Morse manually (record press durations, decode to letters).
- Swap the LED for a small piezo buzzer (or add one alongside it) so the beacon is audible as well as visible.
- Read the message to transmit from `Serial` input instead of a hard-coded string.
