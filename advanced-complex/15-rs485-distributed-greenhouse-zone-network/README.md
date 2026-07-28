# RS-485 Distributed Greenhouse Zone Network

Three Arduino Uno zone controllers each run an independent local PID climate loop for their own greenhouse zone, while a Mega hub polls all zones over RS-485, logs historical data to SD, and serves a dashboard summarizing every zone — a realistic distributed-control pattern where local loops stay fast and autonomous while a supervisory hub only monitors and adjusts targets.

## Board(s) & Roles

- **3x Arduino Uno ("Zone Controller")** — each runs its own local temperature/humidity PID loop for one greenhouse zone (DHT22 + soil sensor + heater/fan/mister relays), continuing to operate even if the hub is offline.
- **Arduino Mega ("Hub")** — RS-485 master; polls all 3 zones' state, can push new setpoints, logs everything to SD with an RTC timestamp, and serves a W5500 Ethernet dashboard.

## Components

| Part | Qty |
|---|---|
| Arduino Uno (zone controller) | 3 |
| Arduino Mega 2560 (hub) | 1 |
| MAX485 RS-485 transceiver module | 4 |
| DHT22 temperature/humidity sensor | 3 |
| Capacitive soil moisture probe | 3 |
| 3-channel relay module (heater, fan, mister per zone) | 3 |
| DS3231 RTC module (I2C, hub) | 1 |
| microSD card module (SPI, hub) + microSD card | 1 |
| W5500 Ethernet shield/module (SPI, hub) | 1 |
| Jumper wires | ~40 |

## Architecture

Each zone controller runs its PID loop entirely locally, independent of the RS-485 bus — this is the key distributed-control design decision: a hub outage or bus fault never stops any zone's climate control. The hub polls each zone in turn every 10 seconds for its current readings/actuator states, logs them to SD, and can push a setpoint update to a specific zone at any time (e.g. from an operator using the dashboard); zones simply adopt whatever setpoint they last received, defaulting to a safe built-in value if none has ever arrived. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

### Zone Controller (Uno x3)

| Arduino Pin | Component | Notes |
|---|---|---|
| D2 | MAX485 DE+RE | |
| D8, D9 | MAX485 RO, DI (`SoftwareSerial`) | |
| D3 | DHT22 DATA | |
| A0 | Soil moisture probe | |
| D4, D5, D6 | Relay IN (heater, fan, mister) | |

### Hub (Mega)

| Arduino Mega Pin | Component | Notes |
|---|---|---|
| D2 | MAX485 DE+RE | |
| D8, D9 | MAX485 RO, DI (`SoftwareSerial`) | |
| 20/21 (SDA/SCL) | DS3231 RTC | |
| 53 (SS) | SD module CS | |
| 49 | Ethernet (W5500) CS | shares SPI bus with SD |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

RS-485 half-duplex, 9600 baud via `SoftwareSerial`, a simplified Modbus-RTU-*inspired* request/response (not full Modbus compliance):

- Hub → zone: `POLL <zoneId>\n` → zone replies `D,<zoneId>,<temp>,<hum>,<soil>,<heater>,<fan>,<mist>,<chk>\n`.
- Hub → zone: `SET <zoneId> <tempSetpoint> <humSetpoint>\n` → zone replies `OK <zoneId>\n` and adopts the new setpoints for its local PID loop immediately.
- Poll cycle: all 3 zones every 10 seconds; setpoint pushes happen out-of-band whenever the dashboard issues one.

HTTP (hub only): `GET /api/zones` → JSON array of zone status; `GET /api/zones/<id>/setpoint?t=NN&h=NN` → pushes a new setpoint to that zone over RS-485.

## Setup & Deployment

1. Wire each zone controller's sensors, relays, and MAX485 transceiver; wire the hub's MAX485, RTC, SD, and Ethernet shield. Daisy-chain all 4 boards' A/B RS-485 lines with a shared ground.
2. Install `SoftwareSerial`, `DHT sensor library`, `RTClib`, `SD`, and `Ethernet` (see `libraries.txt`).
3. Flash `src/zone-controller/zone_controller.ino` to each Uno, setting each one's `MY_ZONE_ID` constant to 1, 2, or 3 respectively.
4. Flash `src/hub/hub_controller.ino` to the Mega.
5. Power all 4 boards; open each zone controller's own Serial Monitor briefly to confirm its local PID loop is running independently of the hub.
6. Confirm the hub's dashboard shows all 3 zones' live readings, and that pushing a setpoint change from the dashboard visibly changes that zone's actuator behavior within a control cycle or two.
7. Disconnect the hub entirely and confirm all 3 zones continue climate-controlling normally — proving the distributed/local-loop design.

## Known Limitations & Path to Production

- Setpoints pushed to a zone are held only in that zone's RAM — a zone reset reverts to its compiled-in default setpoint until the hub next pushes an update.
- The "Modbus-RTU-like" protocol is a simplified, non-compliant subset built for this repo, not interoperable with off-the-shelf Modbus equipment.
- No bus-level retry/backoff tuning beyond a single retry, same as this tier's other RS-485 projects.

## Extension Ideas

- Persist each zone's last-received setpoint to EEPROM so it survives a zone-controller reset.
- Add a real Modbus-RTU library for interoperability with commercial greenhouse sensors/actuators.
- Extend to more zones by only changing the hub's poll list — no protocol changes needed.
