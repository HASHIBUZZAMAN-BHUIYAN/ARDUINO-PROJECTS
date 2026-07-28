# Architecture — Dual-Uno Elevator PLC Simulation

## System Diagram

```
 +------------------------+
 |  Call Panel (Uno)       |
 |  - floor call buttons    |
 |  - 7-seg display         |
 |  - direction LEDs        |
 +------------+-------------+
              | I2C (5Hz poll + call writes)
              v
 +------------------------------+
 |  Car Controller (Uno, 0x20)    |
 |  - state machine                |
 |  - call queue (SCAN-like order) |
 |  - motor drive                   |
 +------------+-------------------+
              v
    +--------------------+
    | Floor limit switches |
    | (closed-loop stop)    |
    +--------------------+
```

## Data Flow

1. **Input (call panel)** — a floor-call button press is debounced and written to the car controller over I2C.
2. **Queue (car controller)** — the requested floor is added to an internal pending-calls set (if not already the current floor).
3. **Decide** — `pickNextTarget()` selects the next floor to service, preferring a pending call in the current direction of travel over one requiring a direction reversal.
4. **Move (closed loop)** — the car controller energizes the up/down motor relay and drives until the target floor's limit switch triggers, which immediately stops the motor — position control anchored to a physical sensor, not a timer.
5. **Door cycle** — on arrival, the state machine holds `DOOR_OPEN` for a fixed dwell time before returning to `IDLE`.
6. **Report** — the call panel polls the car's `{currentFloor, state, doorOpen}` register at 5Hz and updates its display/LEDs accordingly.

## Component Roles

- **Car controller** — the sole motion authority and state-machine owner; the call panel never commands motion directly, only requests a floor.
- **Call panel** — a passenger-facing I/O client with no control authority of its own beyond queueing a call.
- **Limit switches** — the physical ground truth the state machine's `MOVING_UP`/`MOVING_DOWN` states terminate against.
- **I2C link** — the sole coupling between the two boards' distinct responsibilities.
