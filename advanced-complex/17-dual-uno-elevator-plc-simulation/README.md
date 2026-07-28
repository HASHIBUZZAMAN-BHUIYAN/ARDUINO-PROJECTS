# Dual-Uno Elevator PLC Simulation

Two Arduino Unos simulate a small industrial elevator controller: a "car controller" runs a closed-loop floor-position state machine using physical limit switches, while a "call panel" polls the car's state over I2C to drive floor-call buttons and 7-segment displays — modeling a simplified real elevator PLC's split between motion control and passenger-facing UI.

## Board(s) & Roles

- **Arduino Uno ("Car Controller", I2C slave `0x20`)** — drives the elevator motor up/down between 3 floor limit switches, implementing an `IDLE`/`MOVING_UP`/`MOVING_DOWN`/`DOOR_OPEN` state machine and a simplified elevator-algorithm call queue.
- **Arduino Uno ("Call Panel", I2C master)** — reads 3 floor-call buttons, shows the current floor on a 7-segment display and direction on LEDs, and forwards call requests to the car controller.

## Components

| Part | Qty |
|---|---|
| Arduino Uno (car controller) | 1 |
| Arduino Uno (call panel) | 1 |
| Mechanical limit switch (floor position sensing) | 3 |
| DC gear motor + drive mechanism (or a demo servo/motor standing in for the "car") | 1 |
| Relay module or motor driver (car motor direction) | 1 |
| Push button (floor call, x3) | 3 |
| 7-segment display (+ driver, e.g. a shift register) | 1 |
| LED (direction indicator, up/down) | 2 |
| Jumper wires | ~20 |

## Architecture

The car controller owns the only real motion authority: it drives the motor up or down until the physical limit switch for the target floor trips (closed-loop position control against a physical sensor, not a timer), and internally queues multiple pending floor calls, servicing them in current-direction-of-travel order first (a simplified version of the real "elevator algorithm"/SCAN scheduling used by actual elevator PLCs, to avoid needless direction reversals). The call panel is purely a polling/UI client: it reads the car's status register over I2C at 5Hz to keep its floor display and direction LEDs current, and writes a floor-call request whenever a button is pressed. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

### Car Controller (Uno, I2C slave 0x20)

| Arduino Pin | Component | Notes |
|---|---|---|
| A4 (SDA), A5 (SCL) | I2C bus to call panel | |
| D2, D3, D4 | Floor 1/2/3 limit switches | `INPUT_PULLUP` |
| D5, D6 | Motor relay/driver (up, down) | |
| D7 | Door-open indicator relay/LED | |

### Call Panel (Uno, I2C master)

| Arduino Pin | Component | Notes |
|---|---|---|
| A4 (SDA), A5 (SCL) | I2C bus to car controller | |
| D2, D3, D4 | Floor 1/2/3 call buttons | `INPUT_PULLUP` |
| D5, D6, D7 (+ shift register) | 7-segment floor display | |
| D8, D9 | Up/down direction LEDs | |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

I2C, call panel as master, car controller as slave at `0x20`:

- Call panel → car: `Wire.requestFrom(0x20, 3)` returns `{currentFloor, state, doorOpen}`.
- Call panel → car: `Wire.beginTransmission(0x20); Wire.write(requestedFloor); Wire.endTransmission();` queues a floor call.
- Poll rate: 5Hz from the call panel; the car controller's own state machine runs independently at its own pace, not gated by polls.

## Setup & Deployment

1. Wire the car controller's limit switches and motor driver; wire the call panel's buttons, 7-segment display, and direction LEDs.
2. Wire the shared I2C bus between the two boards (A4/A5 on both) plus a common ground.
3. No external libraries needed beyond built-ins (see `libraries.txt`).
4. Flash `src/car-controller/car_controller.ino` to the car controller Uno, then `src/call-panel/call_panel.ino` to the call panel Uno.
5. Power both boards; confirm the car controller reports `IDLE` at whichever floor its limit switch is currently triggered at (or drives to floor 1 as a home position on boot).
6. Press a call button on the call panel and confirm the car controller's state changes to `MOVING_UP`/`MOVING_DOWN`, the direction LED lights accordingly, and the motor stops (state returns to `IDLE`, then briefly `DOOR_OPEN`) once the target floor's limit switch trips.
7. Press two different floor calls in quick succession and confirm the car services them in direction-of-travel order rather than strictly first-come-first-served.

## Known Limitations & Path to Production

- Only 3 floors and one limit switch per floor — a production elevator would use a more precise encoder-based position system plus redundant safety-rated limit switches (this is explicitly a simulation, not a safety-certified control system).
- No door-obstruction sensing (e.g. a real elevator's door safety edge) — `DOOR_OPEN` is purely timed.
- A single I2C bus fault would desync the call panel's display from the car's real state until the next successful poll; production would add a CRC or sequence check on the I2C read.

## Extension Ideas

- Add an emergency-stop button on the call panel that the car controller treats as highest priority over any queued call.
- Add a second call panel (e.g. one per floor) polling the same car controller address.
- Log every floor-call and door-cycle event to SD with an RTC timestamp for a simple "usage audit trail," reusing the SD-logging pattern from other projects in this tier.
