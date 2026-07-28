# Dual-Board CNC: Motion Controller + UI Controller Split

A Mega dedicated purely to motion (3-axis stepping, closed-loop limit-switch homing, spindle relay) is paired with an Uno dedicated to UI and program feeding (SD G-code reading, LCD status, serial queueing), talking over a flow-controlled UART protocol — mirroring how real CNC firmware/UI stacks are architected.

## Board(s) & Roles

- **Arduino Mega 2560 ("Motion Controller")** — owns all real-time motion: 3-axis `AccelStepper` control, interrupt-driven limit-switch homing, and the spindle relay. Never touches SD or the LCD.
- **Arduino Uno ("UI Controller")** — reads G-code lines from an SD card, shows status on a 16x2 LCD, and feeds one motion command at a time to the Mega, only sending the next once the Mega acknowledges the previous is complete.

## Components

| Part | Qty |
|---|---|
| Arduino Mega 2560 | 1 |
| Arduino Uno | 1 |
| A4988/DRV8825 stepper driver | 3 |
| NEMA17 stepper motor | 3 |
| Mechanical limit switch | 3 |
| Relay module (spindle) | 1 |
| microSD card module (SPI, UI board) + microSD card | 1 |
| 16x2 character LCD (UI board) | 1 |
| Jumper wires | ~30 |

## Architecture

The UI controller reads one G-code line at a time from an SD-stored program, converts it to a compact binary motion packet, and sends it to the motion controller over UART; the motion controller only reports `ACK` once that exact move (and any prior queued moves) has fully finished, which is the UI controller's cue to send the next line — this flow control is what prevents the UI board from ever overrunning the motion board's single-move execution model. `G28` triggers the motion controller's own closed-loop homing routine independent of the UI board. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

### Motion Controller (Mega)

| Arduino Mega Pin | Component | Notes |
|---|---|---|
| D2, D3, D18 (INT) | X/Y/Z limit switches | hardware interrupts |
| D22/23, D24/25, D26/27 | X/Y/Z STEP/DIR | to 3 A4988 drivers |
| D28 | Spindle relay IN | |
| D16 (TX2), D17 (RX2) | UART to UI controller (`Serial2`) | keeps `Serial` (USB) free for debugging |

### UI Controller (Uno)

| Arduino Pin | Component | Notes |
|---|---|---|
| D4 (RX), D10 (TX) | UART to motion controller (`SoftwareSerial`) | crossed to the Mega's Serial2 (Uno D4<-Mega D16 TX2, Uno D10->Mega D17 RX2) |
| D9 (SS), D11/D12/D13 | SD module (SPI) | D11/D12/D13 are the Uno's fixed hardware SPI pins |
| D2, D3, D5, D6, D7, D8 | 16x2 LCD RS, EN, D4, D5, D6, D7 | |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

UART at 115200 baud (Uno via `SoftwareSerial`, Mega via hardware `Serial2`), binary framed packet:

`[0x7E][cmdId][x_i16][y_i16][z_i16][feedRate_u16][crc8][0x7F]`

- The motion controller only accepts a new packet once idle (all axes at `distanceToGo() == 0`); it replies `ACK,<cmdId>\n` when idle and ready, or ignores frames received while busy (the UI controller waits for `ACK` before sending, so this should not occur in normal operation).
- The motion controller also streams `POS,<x>,<y>,<z>,<state>\n` status lines every 250ms for the UI controller's LCD.
- `cmdId=0` is reserved for `G28` (home); its packet's X/Y/Z fields are ignored.

## Setup & Deployment

1. Wire the motion controller's steppers, limit switches, and spindle relay; wire the UI controller's SD module and LCD; connect the two boards' UART pins (Mega Serial2 RX2/TX2 to Uno's SoftwareSerial TX/RX, crossed) plus a shared ground.
2. Install `AccelStepper` (motion board) and `SD` + `LiquidCrystal` (UI board) — see `libraries.txt`.
3. Place a `.gcode` program at the SD root as `PROGRAM.GCO`.
4. Flash `src/motion-controller/motion_controller.ino` to the Mega first, then `src/ui-controller/ui_controller.ino` to the Uno.
5. Power both boards. The UI controller's LCD should show "Ready" once it detects the motion controller is responsive.
6. Press the UI controller's start button (or send `RUN` over its Serial Monitor) to begin streaming `PROGRAM.GCO`; confirm the LCD position display updates in step with actual motion and that lines are only sent one at a time (visible via each board's own USB Serial debug output).

## Known Limitations & Path to Production

- The UART link is a single point of failure with no reconnect/resume logic — a disconnected cable mid-program requires restarting the job from the top.
- No acceleration-aware look-ahead across queued moves (each decelerates to zero before the next begins), same limitation as the single-board CNC project in this tier.
- No checksummed/retried delivery beyond the CRC8 — a corrupted packet is currently just dropped, not retried automatically.

## Extension Ideas

- Add automatic retry/resend on CRC8 mismatch instead of silently dropping frames.
- Add a persistent "resume from line N" capability on the UI controller after a fault or reset.
- Split the spindle relay onto its own board entirely for even further separation of concerns (motion vs. tooling vs. UI).
