# Architecture — Master/Slave Serial Command Queue Warehouse AGV

## System Diagram

```
 +----------------+
 |  4x4 Keypad     |
 +--------+-------+
          v
 +------------------------+
 |  Uno "Dispatch Console"  |
 |  - job queue              |
 |  - waits for ACK/DONE     |
 +------------+-------------+
              | UART (115200, framed binary + CRC8)
              v
 +------------------------------+
 |  Mega "AGV"                    |
 |  - line-following (closed loop) |
 |  - executes one job at a time    |
 +----+----------+-------------+---+
      v          v
 +--------+  +--------+
 | IR array|  | Motors |
 +--------+  +--------+
```

## Data Flow

1. **Input** — an operator enters a destination station number on the keypad and confirms with `#`.
2. **Queue** — the console pushes `{jobId, destinationStation}` onto its FIFO queue and updates the LCD.
3. **Dispatch** — once the AGV is idle (no job currently in flight), the console pops the next job, sends it as a framed packet, and waits for `ACK,<jobId>`.
4. **Execute** — the AGV reads the IR array's line-position error every control tick and applies closed-loop PID steering correction to the motors, watching for the destination station's marker pattern.
5. **Complete** — on reaching the marker, the AGV stops and sends `DONE,<jobId>`.
6. **Advance** — the console marks the job complete on its LCD and, if more jobs are queued, dispatches the next one.

## Component Roles

- **Dispatch console** — the sole source of job ordering and the only thing an operator interacts with.
- **AGV** — a single-job-at-a-time executor with no queueing of its own; queueing lives entirely on the console side.
- **UART link** — the sole coordination point; its ACK/DONE handshake is what makes the console's queue flow-controlled rather than blindly stream jobs.
- **IR line array + PID** — the AGV's only navigation sensor/control loop, kept intentionally simple (line-following) so the project's complexity stays focused on the queueing protocol.
