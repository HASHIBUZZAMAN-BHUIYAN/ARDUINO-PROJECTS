# Digital Dice

Press a button to "roll" a virtual six-sided die; the result lights up in the classic dot pattern on seven LEDs arranged like a real die face.

## Difficulty & Board

**Tier:** Basic
**Board:** Arduino Uno Q

Reasoning: seven LEDs plus one button is a few more pins than the most minimal basic project but still small and self-contained — a good fit for the Uno Q's compact form factor for a little desk gadget. As with the other Uno Q project in this repo, all I/O here runs at **3.3V logic**.

## Components

| Part | Qty |
|---|---|
| Arduino Uno Q | 1 |
| LED (5mm) | 7 |
| 220 Ω resistor | 7 |
| Pushbutton | 1 |
| 10 kΩ resistor (button pull-down) | 1 |
| Breadboard | 1 |
| Jumper wires | ~12 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| D2 | LED 1 (top-left dot) via 220 Ω | |
| D3 | LED 2 (top-right dot) via 220 Ω | |
| D4 | LED 3 (middle-left dot) via 220 Ω | |
| D5 | LED 4 (center dot) via 220 Ω | |
| D6 | LED 5 (middle-right dot) via 220 Ω | |
| D7 | LED 6 (bottom-left dot) via 220 Ω | |
| D8 | LED 7 (bottom-right dot) via 220 Ω | |
| D12 | Pushbutton (one side) | other side to 3V3, plus 10kΩ pull-down to GND on the D12 side |
| GND | All LED cathodes | shared ground |

Physical layout of the 7 LEDs on a breadboard mimics a standard die face (3x3 grid, corners + center + two middle-row sides):

```
1   2
  4
3   5
6   7
```

## How It Works

Seven LEDs are enough to represent every face of a standard die (1-6) using the usual dot layout, where each face number lights a specific subset of the seven positions (e.g. face "1" only lights the center LED; face "6" lights all four corners plus both middle-row LEDs, skipping the center).

The sketch stores that mapping as a 6x7 table of booleans (`DICE_PATTERNS[face][led]`). On each confirmed button press:

1. It seeds `random()` using `analogRead()` on a floating/unused analog pin — a common trick to get a less-predictable seed than always starting from the same value.
2. It briefly flashes through a few random faces in quick succession (a "rolling" animation) before settling on the final random result (1-6).
3. It lights exactly the LEDs in that face's pattern and holds until the next press.

This is the "one core concept" of the project: mapping a random integer onto a fixed pattern table, plus basic button debouncing.

## Setup & Flashing

1. Wire all seven LEDs and the pushbutton as above, arranged in the 3x3 die layout for a satisfying visual.
2. Open `src/digital_dice.ino` in the Arduino IDE.
3. Select **Tools > Board > Arduino Uno Q** and the correct COM port.
4. Upload — no external libraries required.
5. Press the button repeatedly and confirm each roll settles on a valid, correctly-patterned face 1-6.

## Extensions

- Add a second button for "roll two dice" (duplicate the pattern with a second LED set), useful for board games.
- Add a small speaker for a dice-rolling sound effect during the animation.
- Replace the 7 discrete LEDs with a small 8x8 LED matrix and MAX7219 driver for a fancier single-component display.
