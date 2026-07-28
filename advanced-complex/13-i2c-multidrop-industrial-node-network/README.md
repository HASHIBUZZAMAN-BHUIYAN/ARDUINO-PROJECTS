# I2C Multi-Drop Industrial Node Network

An Uno Q acts as I2C master/hub over a shared bus to 3 Arduino Nano slave nodes — a relay bank, a load-cell scale, and a K-type thermocouple reader — polling each in turn, logging readings to SD, and serving a dashboard with relay toggle controls.

## Board(s) & Roles

- **Arduino Uno Q ("Hub")** — I2C master; polls each of the 3 Nano nodes in turn, logs to SD with an RTC timestamp, and serves a W5500 Ethernet dashboard.
- **Arduino Nano ("Relay Bank", I2C address `0x08`)** — a 4-relay actuator node, commanded by the hub.
- **Arduino Nano ("Load Cell", I2C address `0x09`)** — an HX711 + load cell weight sensor node.
- **Arduino Nano ("Thermocouple", I2C address `0x0A`)** — a MAX6675 K-type thermocouple reader node.

## Components

| Part | Qty |
|---|---|
| Arduino Uno Q (hub) | 1 |
| Arduino Nano (node) | 3 |
| 4-channel relay module | 1 |
| HX711 load cell amplifier + load cell | 1 |
| MAX6675 thermocouple amplifier + K-type thermocouple | 1 |
| DS3231 RTC module (I2C, hub) | 1 |
| microSD card module (SPI, hub) + microSD card | 1 |
| W5500 Ethernet shield/module (SPI, hub) | 1 |
| Jumper wires | ~30 |

## Architecture

All 4 boards share one I2C bus. The hub polls each node's address in turn via `Wire.requestFrom()`, receiving a small binary struct specific to that node type (relay-state bitmask, weight reading, or temperature reading); to actuate a relay, the hub instead issues `Wire.write()` to the relay node's address with a new bitmask. Every successful poll is logged to SD with an RTC timestamp, and a W5500-hosted dashboard shows live values for all 3 nodes plus relay toggle buttons. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

### Hub (Uno Q)

| Arduino Pin | Component | Notes |
|---|---|---|
| A4 (SDA), A5 (SCL) | I2C bus to all 3 nodes | shared bus, pull-up resistors (4.7kΩ) at one point on the bus |
| A4/A5 also | DS3231 RTC | shares the same I2C bus (distinct address, no conflict) |
| D9 (SS) | SD module CS | |
| D7 | Ethernet (W5500) CS | |

### Each Nano Node

| Arduino Pin | Component | Notes |
|---|---|---|
| A4 (SDA), A5 (SCL) | I2C bus | address set in software (`Wire.begin(0x08)` etc.) |
| (Relay node) D4-D7 | 4-channel relay IN1-IN4 | |
| (Load cell node) D2, D3 | HX711 DOUT, SCK | |
| (Thermocouple node) D8 (CS), D11 (SO), D13 (SCK) | MAX6675 | software SPI pins per the MAX6675 library |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

I2C, 100kHz standard mode, hub as master:

- Every 2 seconds, hub calls `Wire.requestFrom(addr, len)` for each of the 3 node addresses (`0x08`, `0x09`, `0x0A`) and reads a fixed-size struct back.
- To change relay state, the hub calls `Wire.beginTransmission(0x08); Wire.write(bitmask); Wire.endTransmission();`.
- **Known caveat:** I2C is not designed for long multi-drop cable runs off a breadboard/protoboard — keep total bus length short (well under a meter) or this pattern becomes unreliable; see Known Limitations.

## Setup & Deployment

1. Wire all 3 Nano nodes and the hub onto one shared I2C bus (SDA-SDA, SCL-SCL, GND-GND across all 4 boards) with a single 4.7kΩ pull-up pair on the bus.
2. Wire each node's specific hardware (relay module, HX711 + load cell, MAX6675 + thermocouple).
3. Install `HX711` (by bogde) and `MAX6675` libraries on the relevant Nanos; install `RTClib`, `SD`, `Ethernet` on the hub (see `libraries.txt`).
4. Flash `src/node-relay-bank/node_relay_bank.ino`, `src/node-load-cell/node_load_cell.ino`, and `src/node-thermocouple/node_thermocouple.ino` to their respective Nanos.
5. Flash `src/hub/hub_controller.ino` to the Uno Q.
6. Power all 4 boards; open the hub's Serial Monitor at 9600 baud to confirm all 3 nodes respond to polls.
7. Browse to the hub's dashboard URL, confirm live weight/temperature readings, and use the relay toggle buttons to confirm round-trip control.

## Known Limitations & Path to Production

- I2C over a shared multi-drop bus is fragile beyond short, well-shielded runs — a real industrial deployment of this pattern would use RS-485 instead (see this tier's other RS-485 mesh projects) for the physical layer, keeping the same "hub polls addressed nodes" logical pattern.
- No bus-level authentication; any device on the bus could impersonate a node's address.
- A single stuck/hung node can potentially stall the I2C bus for all others (a classic I2C failure mode) — production would add a bus-recovery routine (toggling SCL to unstick a slave holding SDA low).

## Extension Ideas

- Add a bus-recovery/watchdog routine on the hub that detects and clears a stuck I2C bus.
- Migrate the physical layer to RS-485 while keeping the same addressed-polling logical protocol, for longer and more reliable cable runs.
- Add a 4th node type (e.g. a strain-gauge vibration sensor) to demonstrate the address scheme scaling further.
