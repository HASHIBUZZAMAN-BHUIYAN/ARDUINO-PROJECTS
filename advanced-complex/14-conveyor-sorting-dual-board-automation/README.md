# Conveyor Sorting Dual-Board Automation

An Uno running a color sensor and diverter servo acts as an I2C slave "sorter," while a Mega runs the conveyor motor with closed-loop PID belt-speed control and an object counter as the I2C master "line controller" — together forming a small coordinated sorting line.

## Board(s) & Roles

- **Arduino Uno ("Sorter", I2C slave `0x10`)** — reads a TCS3200 color sensor and swings a diverter servo to the commanded lane.
- **Arduino Mega ("Line Controller", I2C master)** — drives the conveyor motor with closed-loop PID speed control (using an IR break-beam pulse rate as feedback), counts objects passing, and commands the sorter's diverter based on the sorter's reported color.

## Components

| Part | Qty |
|---|---|
| Arduino Uno (sorter) | 1 |
| Arduino Mega 2560 (line controller) | 1 |
| TCS3200 color sensor | 1 |
| Micro servo (diverter gate) | 1 |
| DC gear motor (conveyor belt) | 1 |
| L298N (or similar) motor driver | 1 |
| IR break-beam sensor pair (object counting / speed feedback) | 1 |
| Jumper wires | ~20 |

## Architecture

The Mega line controller runs a closed-loop PID adjusting the conveyor motor's PWM duty cycle to hold a constant belt speed, inferred from the IR break-beam's pulse rate as items (or a marked belt reference point) pass. When an item reaches the sorter's position, the Mega requests the sorter's most recently detected color over I2C and immediately commands a diverter lane back over I2C; the sorter swings its servo to that lane. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

### Sorter (Uno, I2C slave 0x10)

| Arduino Pin | Component | Notes |
|---|---|---|
| A4 (SDA), A5 (SCL) | I2C bus to Mega | |
| D2, D3, D4, D5 | TCS3200 S0-S3 | frequency-scaling selects |
| D6 | TCS3200 OUT | read via `pulseIn()` |
| D9 | Diverter servo signal | |

### Line Controller (Mega, I2C master)

| Arduino Mega Pin | Component | Notes |
|---|---|---|
| 20/21 (SDA/SCL) | I2C bus to Uno | Mega's dedicated I2C pins |
| D8, D9 | L298N IN1, IN2 (conveyor motor) | |
| D10 (PWM) | L298N ENA (speed) | PID output |
| D2 (INT) | IR break-beam OUT | interrupt-counted for speed feedback + item counting |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

I2C, 100kHz, Mega as master, Uno as slave at address `0x10`:

- Mega → Uno: `Wire.requestFrom(0x10, 1)` returns the sorter's last-detected color as a single byte enum (`0=unknown, 1=red, 2=green, 3=blue`).
- Mega → Uno: `Wire.beginTransmission(0x10); Wire.write(laneByte); Wire.endTransmission();` commands the diverter to a lane (`0=straight, 1=left, 2=right`).

## Setup & Deployment

1. Wire the sorter's TCS3200 and diverter servo; wire the line controller's motor driver and IR break-beam.
2. Wire the shared I2C bus between the two boards (Uno A4/A5 to Mega 20/21) plus a common ground.
3. Install `Servo` (built-in) on the Uno; no extra libraries needed on the Mega beyond built-ins (see `libraries.txt`).
4. Flash `src/sorter-uno/sorter.ino` to the Uno, then `src/line-controller-mega/line_controller.ino` to the Mega.
5. Power both boards. Confirm the conveyor motor ramps to and holds its target speed (visible via the Mega's Serial Monitor PID debug output).
6. Pass a colored object across the TCS3200 and confirm the diverter swings to the corresponding lane as it reaches the sorter position.

## Known Limitations & Path to Production

- No encoder on the conveyor motor itself — belt speed feedback comes from the IR break-beam's pulse rate against a fixed marker, which is a coarser signal than a dedicated shaft encoder.
- The color→lane mapping is fixed in code; a production line would make this operator-configurable (e.g. via the sorter's own small UI or the hub dashboard pattern used elsewhere in this tier).
- No jam detection (e.g. a stalled belt under load) distinct from a slow-but-moving belt.

## Extension Ideas

- Add a shaft encoder on the conveyor motor for tighter closed-loop speed control.
- Add a reject/error lane for unrecognized colors instead of defaulting to "straight."
- Log sorted-item counts per lane to SD for a simple production-count dashboard, reusing the SD+dashboard pattern from other projects in this tier.
