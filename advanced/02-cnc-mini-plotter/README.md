# Stepper Motor CNC Mini Plotter

A 2-axis (X/Y) pen plotter driven by two stepper motors through A4988 drivers, with a servo that lifts the pen for travel moves. It reads a tiny G-code-like command language over serial so you can send simple line-drawing programs from a computer.

## Difficulty & Board

**Tier:** Advanced
**Board:** Arduino Mega 2560

Reasoning: two stepper drivers (2 pins each for step/dir, ideally plus enable = up to 6 pins), a pen-lift servo, and optional limit switches for homing adds up quickly, and stepper-heavy CNC-style projects are exactly the "many simultaneous timing-critical outputs" case the Mega's extra pins and this repo's board-rotation guidance both favor over an Uno.

## Components

| Part | Qty |
|---|---|
| Arduino Mega 2560 | 1 |
| A4988 stepper driver module | 2 |
| NEMA17 stepper motor | 2 |
| SG90 micro servo (pen lift) | 1 |
| 12V power supply (for stepper motors) | 1 |
| X/Y plotter frame (belt + rod, or leadscrew) kit | 1 |
| Limit switches (optional, for homing) | 2 |
| Jumper wires | ~14 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| D2 | X-axis driver STEP | |
| D3 | X-axis driver DIR | |
| D4 | X-axis driver ENABLE | active-LOW on most A4988 boards |
| D5 | Y-axis driver STEP | |
| D6 | Y-axis driver DIR | |
| D7 | Y-axis driver ENABLE | |
| D9 | Pen-lift servo signal | |
| D30 | X-axis limit switch (optional) | to GND, `INPUT_PULLUP` |
| D31 | Y-axis limit switch (optional) | to GND, `INPUT_PULLUP` |
| 12V supply | A4988 VMOT (both drivers) | motor power, separate from Mega logic supply |
| 5V | A4988 VDD (both drivers), servo VCC | logic power |

## How It Works

Each A4988 driver converts simple **step + direction** signals into the correct 4-phase current sequence for its NEMA17 motor — every pulse on `STEP` advances the motor one microstep, and the `DIR` pin's level chooses which way. The `AccelStepper` library handles generating properly-timed step pulses (with acceleration/deceleration ramps so the motors don't stall by starting at full speed instantly) for both axes independently, letting the sketch just say "go to X=120, Y=80" and have both motors arrive smoothly together.

The sketch implements a minimal command language read line-by-line over `Serial`, deliberately small rather than a full G-code interpreter:

- `G0 X<mm> Y<mm>` — pen-up travel move to the given position.
- `G1 X<mm> Y<mm>` — pen-down drawing move to the given position.
- `PENUP` / `PENDOWN` — move the servo without moving the axes.
- `HOME` — (if limit switches are wired) drives both axes toward their switches until triggered, then zeroes the coordinate system there.

Millimeters are converted to motor steps using a `STEPS_PER_MM` constant that depends on your specific pulley/belt or leadscrew pitch — this is the standard technique every real CNC/3D-printer firmware uses to keep the human-facing coordinate system independent of the mechanical stepping details.

## Setup & Flashing

1. Assemble the X/Y frame, mount both NEMA17 motors and the pen-lift servo, and wire the A4988 drivers as above. **Set each A4988's current-limit trimmer potentiometer** per your motor's rated current before connecting motors (too high a limit can overheat the driver or motor).
2. Install the **AccelStepper** library via Library Manager.
3. Open `src/cnc_plotter.ino`, and adjust `STEPS_PER_MM` to match your hardware (pulley tooth count / belt pitch, or leadscrew pitch).
4. Select **Tools > Board > Arduino Mega or Mega 2560** and the correct COM port, then upload.
5. Open the Serial Monitor (or a terminal program) at 9600 baud, set line-ending to Newline, and send test commands like `G0 X10 Y10` then `G1 X50 Y10` to confirm axis motion and pen lift both work before attempting a full drawing.

## Extensions

- Write a small desktop script that converts an SVG path into a sequence of `G0`/`G1` commands and streams them over serial automatically.
- Add the optional limit switches and implement the `HOME` command for repeatable startup positioning.
- Swap the pen holder for a low-power laser module (with appropriate safety precautions) to make a mini engraver instead of a plotter.
