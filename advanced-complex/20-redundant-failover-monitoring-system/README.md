# Redundant Dual-MCU Failover Monitoring System

Two Arduino Unos — Primary and Standby — are wired in parallel to the same critical sensors (temperature, gas/smoke, water leak) and the same alarm actuators. Primary normally drives the alarms while sending a heartbeat; if Standby stops hearing that heartbeat, it takes over control within about a second, and yields back automatically once Primary returns — a redundancy/failover pattern with an explicit split-brain mitigation.

## Board(s) & Roles

- **Arduino Uno ("Primary")** — normally the sole active controller: reads all 3 sensors, drives the siren/valve relays, and transmits a heartbeat.
- **Arduino Uno ("Standby")** — listens for Primary's heartbeat; if it goes silent for over 1 second, Standby self-promotes to active (starts driving the same sensors/actuators independently) and begins transmitting its own heartbeat, so a re-awakened Primary can detect it's no longer the active unit and yield.

## Components

| Part | Qty |
|---|---|
| Arduino Uno | 2 |
| DS18B20 temperature sensor (OneWire) | 1 |
| MQ-2 gas/smoke sensor (analog) | 1 |
| Water-leak probe (digital) | 1 |
| Siren relay | 1 |
| Shutoff-valve relay | 1 |
| Jumper wires | ~20 |

## Architecture

All 3 sensors' signal lines and both actuator relays' control lines are physically wired to BOTH boards in parallel; the key safety rule is that a board only ever drives its actuator pins as `OUTPUT` while it is the active unit — the non-active board keeps those same pins as high-impedance `INPUT` so it cannot fight the active board for control of the shared relay lines. Primary starts active by default and transmits a heartbeat over UART every 250ms; Standby listens continuously, and self-promotes only if that heartbeat has been silent for over 1 second. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

### Both Boards (identical, parallel sensor/actuator wiring)

| Arduino Pin | Component | Notes |
|---|---|---|
| D2 | DS18B20 (OneWire) | shared 4.7kΩ pull-up |
| A0 | MQ-2 gas/smoke sensor | |
| D3 | Water-leak probe | `INPUT_PULLUP` |
| D4 | Siren relay IN | `OUTPUT` only while active; `INPUT` while standby |
| D5 | Shutoff-valve relay IN | `OUTPUT` only while active; `INPUT` while standby |
| D6 (TX), D7 (RX) | UART heartbeat link (`SoftwareSerial`) | crossed between the two boards |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

UART heartbeat over `SoftwareSerial` at 9600 baud:

- Whichever board is active sends `HB,<seq>,<state>\n` every 250ms, where `<state>` summarizes current sensor/alarm status.
- The non-active board only listens; if it hasn't heard a heartbeat in over 1000ms, it self-promotes: switches its actuator pins to `OUTPUT`, begins independently sampling its own sensors, and starts sending its own heartbeat.
- **Split-brain mitigation:** a board only ever drives actuator pins while it does not hear a heartbeat sourced from a different, still-active board. Concretely, Primary always starts active and never yields unless it explicitly hears a heartbeat from Standby (which only happens after Standby has self-promoted) — at that point Primary demotes itself to listen-only, preventing both boards from ever driving the shared relay lines simultaneously.

## Setup & Deployment

1. Wire all 3 sensors and both relays to BOTH boards in parallel (identical wiring on each board), plus the crossed UART heartbeat link between them.
2. No external libraries needed beyond `OneWire`/`DallasTemperature` and `SoftwareSerial` (see `libraries.txt`).
3. Flash `src/primary/primary.ino` to one Uno and `src/standby/standby.ino` to the other.
4. Power both boards. Confirm (via each board's own USB Serial Monitor) that Primary reports `ACTIVE` and Standby reports `LISTENING`.
5. Disconnect Primary's power and confirm Standby reports `PROMOTED TO ACTIVE` within about a second, and that it now correctly drives the siren/valve relays in response to simulated sensor events.
6. Reconnect Primary and confirm it detects Standby's heartbeat, logs `YIELDING (standby is active)`, and does not attempt to drive the shared relay lines while Standby remains active.

## Known Limitations & Path to Production

- This is a two-node, single-fault-tolerant design — it does not handle a simultaneous dual failure or a "babbling" board that gets stuck falsely believing it should be active.
- No physical interlock (e.g. diodes or a hardware arbiter chip) backstops the software-only split-brain mitigation — a firmware bug in the promotion logic could theoretically still cause a brief conflict. Production-grade redundancy would add a hardware-level arbitration circuit as a second line of defense.
- Sensor wiring fans out to both boards with no isolation — a short on one board's input pin could affect the shared signal line for both.

## Extension Ideas

- Add a third node as a tie-breaker/voter for a proper 2-of-3 redundant architecture.
- Add a physical "which unit is active" indicator LED pair, visible without a Serial Monitor.
- Log every failover event (promotion/yield) to SD with a timestamp for a maintenance audit trail.
