# Architecture — G-Code CNC Mill Controller

## System Diagram

```
 +----------------+       +---------------------+
 |  Host serial   |       |  SD card (PROGRAM.GCO)|
 |  (G-code line) |       +----------+-----------+
 +--------+-------+                  |
          |                          v
          +------------> +-----------------------+
                          |   G-code line parser  |
                          +-----------+-----------+
                                      v
                          +-----------------------+
                          |  Move queue (ring buf)|
                          +-----------+-----------+
                                      v
             +------------------------------------------+
             |   Mega motion core (AccelStepper x3)      |
             |   - executes queued move                   |
             |   - homing state machine on G28             |
             +------+----------------+----------------+----+
                    v                v                v
              +-----------+   +-----------+    +-----------+
              | X stepper |   | Y stepper |     | Z stepper |
              +-----+-----+   +-----+-----+    +-----+-----+
                    ^                ^                ^
              +-----+-----+   +-----+-----+    +-----+-----+
              |X limit sw |   |Y limit sw |     |Z limit sw |
              +-----------+   +-----------+    +-----------+
```

## Data Flow

1. **Input** — a line arrives either from `Serial.readStringUntil('\n')` or from the open SD file when `RUN` mode is active.
2. **Parse** — `parseLine()` extracts the G/M command and X/Y/Z/F arguments into a `Move` struct.
3. **Queue** — the struct is pushed onto `moveQueue`; the main loop only pops the next entry once all axes report `distanceToGo() == 0`.
4. **Execute** — `stepper[axis].moveTo()` is set per axis; `stepper[axis].run()` is called every loop iteration (non-blocking).
5. **Home (G28 only)** — `homeAxis()` drives toward the limit switch at homing speed, stops on the switch's interrupt-set flag, backs off, then re-approaches slowly for a repeatable zero.
6. **Acknowledge** — once a move completes, `Serial.println("ok")` is sent, which is the queue's flow-control signal to release the next line.

## Component Roles

- **G-code parser** — translates text commands into structured motion requests.
- **Move queue** — decouples "line arrival" from "line execution" so the controller never blocks the host mid-motion.
- **AccelStepper instances** — own per-axis trapezoidal step generation.
- **Limit switches + interrupts** — ground truth for the homing state machine; set a volatile flag consumed by `homeAxis()`.
- **Spindle relay** — actuated directly from `M3`/`M5` commands, independent of motion state.
