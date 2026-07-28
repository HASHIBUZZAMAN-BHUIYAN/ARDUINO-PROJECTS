# G-Code CNC Mill Controller with Closed-Loop Homing

A Mega parses a small G-code subset from SD or serial, drives 3 stepper axes with non-blocking motion, performs closed-loop limit-switch homing, and flow-controls its own command queue so a host never overflows the motion buffer.

## Board

**Arduino Mega 2560** — 3 stepper drivers (2 pins each) + 3 limit switches + a spindle relay + an SD card reader adds up to more I/O than an Uno offers cleanly, and the Mega's extra hardware interrupt pins (2, 3, 18, 19, 20, 21) let all three limit switches use real interrupts instead of polling.

## Components

| Part | Qty |
|---|---|
| Arduino Mega 2560 | 1 |
| A4988/DRV8825 stepper driver | 3 |
| NEMA17 stepper motor | 3 |
| Mechanical limit switch | 3 |
| Relay module (spindle/tool power) | 1 |
| microSD card module (SPI) + microSD card | 1 |
| 12V PSU for stepper drivers | 1 |
| Jumper wires | ~30 |

## Architecture

A program of G-code lines (from SD, or streamed over serial) is parsed one line at a time into a motion command; each command is only dequeued once the previous motion has finished (closed-loop flow control against the stepper buffer), and `G28` triggers a two-stage homing routine (fast approach until a limit switch trips, back off, slow re-approach) for repeatable zeroing. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

| Arduino Mega Pin | Component | Notes |
|---|---|---|
| D2 (INT) | X limit switch | hardware interrupt, pulled up internally |
| D3 (INT) | Y limit switch | hardware interrupt |
| D18 (INT) | Z limit switch | hardware interrupt |
| D22, D23 | X STEP, DIR | to A4988 #1 |
| D24, D25 | Y STEP, DIR | to A4988 #2 |
| D26, D27 | Z STEP, DIR | to A4988 #3 |
| D28 | Spindle relay IN | |
| 53/51/50/52 | SD CS/MOSI/MISO/SCK | hardware SPI |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

No network stack — the protocol here is the serial/SD **command queue**: each parsed G-code line becomes an internal `Move` struct pushed into a small ring buffer. The main loop only calls `stepper.moveTo()` for the next queued move once `stepper.distanceToGo() == 0` for all axes, and prints `ok\n` after consuming each line — mirroring the flow-control handshake real CNC firmware (e.g. grbl) uses so a streaming host (or the on-board SD reader) never sends more than the buffer can hold. Baud rate: 115200.

## Setup & Deployment

1. Wire steppers, drivers, limit switches, spindle relay, and SD module as above; power drivers from a separate 12V supply sharing ground with the Mega.
2. Install `AccelStepper` and `SD` (see `libraries.txt`).
3. Open `src/cnc_controller.ino`, upload to the Mega.
4. Open Serial Monitor at 115200 baud. Send `G28` to home all three axes — confirm each axis approaches its limit switch, backs off 5mm, and re-approaches slowly before reporting `ok`.
5. Send a short manual program (`G1 X10 Y10 F800`, then `G1 X0 Y0`) and confirm smooth, coordinated motion.
6. To run from SD instead, place a `.gcode` file named `PROGRAM.GCO` at the SD root and send `RUN` over serial.

## Known Limitations & Path to Production

- No acceleration-limited path planning across corners (each move decelerates to zero before the next starts) — a production controller would use look-ahead junction-velocity planning.
- Homing is single-speed per stage; no stall-detection (e.g. TMC2209 sensorless homing) as a fallback if a physical switch fails.
- No soft-limits/travel bounds checking against a configured work envelope.

## Extension Ideas

- Add arc support (`G2`/`G3`) via segmented linear approximation.
- Add a probing cycle (`G38.2`) for touch-plate Z-zeroing.
- Stream the queue's remaining-line count back to a host UI for progress display.
