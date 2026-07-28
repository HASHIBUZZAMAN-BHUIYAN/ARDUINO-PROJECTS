# Architecture — Dual-Board CNC: Motion/UI Split

## System Diagram

```
 +----------------+
 |  SD (PROGRAM.  |
 |  GCO)          |
 +--------+-------+
          v
 +------------------------+       +----------------------+
 |  Uno "UI Controller"    |------>|  16x2 LCD status      |
 |  - parses G-code lines  |       +----------------------+
 |  - sends motion packets |
 |  - waits for ACK        |
 +-----------+-------------+
             | UART (115200, framed binary packet)
             v
 +------------------------------+
 |  Mega "Motion Controller"      |
 |  - AccelStepper x3              |
 |  - limit-switch homing           |
 |  - spindle relay                  |
 +----+----------+----------+--------+
      v          v          v
 +--------+ +--------+ +--------+
 | X axis | | Y axis | | Z axis |
 +--------+ +--------+ +--------+
```

## Data Flow

1. **Read** — the UI controller reads the next line of `PROGRAM.GCO` from SD.
2. **Encode** — `encodePacket()` turns the line into a binary `Move` packet with a CRC8.
3. **Send & wait** — the UI controller transmits the packet over `SoftwareSerial` and blocks (with a timeout) until it receives `ACK,<cmdId>` from the motion controller.
4. **Execute** — the motion controller decodes the packet, validates the CRC8, and sets `AccelStepper` targets for all 3 axes (or triggers `homeAllAxes()` for a `G28` packet).
5. **Report** — while executing, the motion controller streams `POS,...` lines every 250ms; once idle again, it sends `ACK,<cmdId>`, which unblocks the UI controller to send the next line.
6. **Display** — the UI controller's LCD shows the latest position and program-line progress.

## Component Roles

- **UI controller (Uno)** — owns program sourcing (SD), operator-facing status (LCD), and command pacing (waits for ACK).
- **Motion controller (Mega)** — owns everything real-time: step generation, homing, and the spindle relay; never touches SD or the LCD.
- **UART link** — the sole coupling between the two boards' responsibilities, deliberately kept to one simple framed protocol.
