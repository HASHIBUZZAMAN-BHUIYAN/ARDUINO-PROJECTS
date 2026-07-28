# Master/Slave Serial Command Queue: Warehouse AGV Dispatch

An Uno "dispatch console" lets an operator queue station-to-station delivery jobs on a keypad and LCD, feeding them one at a time over a checksummed UART protocol to a Mega-based AGV that line-follows between numbered stations with closed-loop PID drive control.

## Board(s) & Roles

- **Arduino Uno ("Dispatch Console")** — 4x4 keypad job entry, 16x2 LCD queue display, holds the local job queue, and only sends the next job once the AGV acknowledges the previous one is complete.
- **Arduino Mega ("AGV")** — IR-array line-following with closed-loop PID drive control, executes one queued job at a time (drive to the destination station, stop, report done).

## Components

| Part | Qty |
|---|---|
| Arduino Uno (dispatch console) | 1 |
| Arduino Mega 2560 (AGV) | 1 |
| 4x4 matrix keypad | 1 |
| 16x2 character LCD | 1 |
| 5-sensor IR line-following array | 1 |
| Dual motor driver (e.g. L298N) | 1 |
| DC gear motor + wheel (x2) + chassis | 1 set |
| Jumper wires | ~25 |

## Architecture

The dispatch console maintains a small in-memory job queue (`{jobId, destinationStation}`); it transmits one job packet at a time over a checksummed UART protocol and will only dequeue and send the next job after receiving either an `ACK` (job delivered, AGV now executing) or having retried and given up. The AGV executes exactly one job at a time: it line-follows with closed-loop PID steering correction (derived from the IR array's line-position error) until it detects the destination station's marker, then reports `DONE` back to the console. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

### Dispatch Console (Uno)

| Arduino Pin | Component | Notes |
|---|---|---|
| D2-D9 | 4x4 keypad rows/columns | |
| D10-D13, A0, A1 | 16x2 LCD RS, EN, D4-D7 | |
| D0 (RX), D1 (TX) — reserved for USB debug; use `SoftwareSerial` on A2/A3 for the AGV link | | |

### AGV (Mega)

| Arduino Mega Pin | Component | Notes |
|---|---|---|
| A8-A12 | 5-sensor IR line array | analog or digital depending on module |
| D8, D9, D10, D11 | L298N IN1-IN4 (left/right motor) | |
| D16 (TX2), D17 (RX2) | UART to dispatch console (`Serial2`) | |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

UART at 115200 baud, binary framed packet: `[STX=0x02][len][cmd][payload...][crc8][ETX=0x03]`

- Console → AGV: `cmd=JOB`, payload `{jobId, destinationStation}`. The console will not send the next queued job until it receives an `ACK,<jobId>` or exhausts its retry budget (2 retries, 1s apart) and reports a dispatch failure on its LCD.
- AGV → console: `ACK,<jobId>` on successful receipt (job accepted, now executing); `DONE,<jobId>` on arrival at the destination station.

## Setup & Deployment

1. Wire the dispatch console's keypad and LCD; wire the AGV's IR line array and motor driver.
2. Connect the two boards' UART lines (console `SoftwareSerial` on A2/A3 crossed to the Mega's `Serial2` on D16/D17) plus a shared ground.
3. No external libraries needed beyond built-ins for the AGV; the console needs a keypad library (see `libraries.txt`).
4. Flash `src/agv/agv.ino` to the Mega first, then `src/dispatch-console/dispatch_console.ino` to the Uno.
5. Set up a line-following track with distinguishable station markers (e.g. a short perpendicular cross-line at each station) at 2-3 points.
6. Power both boards. On the console's keypad, enter a station number and press `#` to queue a job; confirm the LCD shows the queued job, the AGV begins line-following, and the console's LCD updates to show `DONE` once the AGV reports arrival.
7. Queue two jobs in quick succession and confirm the console waits for the first `DONE`-adjacent `ACK` cycle before dispatching the second (visible as the AGV completing one full trip before starting the next).

## Known Limitations & Path to Production

- Only recognizes station markers by a simple cross-line pattern — a production AGV would use RFID floor tags or fiducial markers for unambiguous station identification.
- No obstacle detection on the line-following path — an object blocking the track simply stalls the AGV until manually cleared.
- A permanently lost UART link leaves the console unable to dispatch further jobs; production would add a physical "AGV present/alive" heartbeat independent of job traffic.

## Extension Ideas

- Add an ultrasonic sensor on the AGV for basic obstacle-stop safety.
- Add RFID station tags for unambiguous destination detection instead of line-pattern markers.
- Support multiple AGVs by extending the dispatch console's protocol with an AGV-ID field, dispatching to whichever AGV is idle.
