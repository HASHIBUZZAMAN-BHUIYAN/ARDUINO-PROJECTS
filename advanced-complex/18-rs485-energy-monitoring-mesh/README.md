# RS-485 Energy Monitoring Mesh

Three Arduino Nano meter nodes each measure real power on a different household circuit via a non-invasive CT clamp and an AC voltage sensor, reporting over RS-485 to a Mega hub that accumulates per-circuit kWh, logs historical data to SD, and serves a live per-circuit dashboard.

## Board(s) & Roles

- **3x Arduino Nano ("Meter Node")** — each clamps a non-invasive SCT-013 current sensor around one circuit's live wire and reads a ZMPT101B voltage sensor, computing real power via synchronized voltage/current sampling.
- **Arduino Mega ("Hub")** — RS-485 master; polls all 3 meter nodes, accumulates cumulative watt-hours per circuit, logs to SD with an RTC timestamp, and serves a W5500 Ethernet dashboard.

## Components

| Part | Qty |
|---|---|
| Arduino Nano (meter node) | 3 |
| Arduino Mega 2560 (hub) | 1 |
| MAX485 RS-485 transceiver module | 4 |
| SCT-013 non-invasive current (CT) clamp sensor | 3 |
| ZMPT101B AC voltage sensor module | 3 |
| DS3231 RTC module (I2C, hub) | 1 |
| microSD card module (SPI, hub) + microSD card | 1 |
| W5500 Ethernet shield/module (SPI, hub) | 1 |
| Jumper wires | ~30 |

## Architecture

Each meter node samples both its CT clamp and voltage sensor at a fast, synchronized rate over a mains cycle to compute real power (accounting for phase relationship between voltage and current, not just their peak magnitudes), accumulating watt-seconds into a running cumulative watt-hour total that survives node resets is out of scope here (see Known Limitations) but persists for the runtime of the sketch. The hub polls each node every 5 seconds over RS-485 for instantaneous watts and cumulative Wh, logs both to SD, and serves a dashboard with live per-circuit power and simple daily-total charts. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

### Meter Node (Nano x3)

| Arduino Pin | Component | Notes |
|---|---|---|
| D2 | MAX485 DE+RE | |
| D8, D9 | MAX485 RO, DI (`SoftwareSerial`) | |
| A0 | SCT-013 CT clamp (via burden resistor) | |
| A1 | ZMPT101B voltage sensor output | |

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

RS-485 half-duplex, 9600 baud via `SoftwareSerial`:

- Hub → node: `POLL <nodeId>\n`.
- Node → hub: `D,<nodeId>,<instantWatts>,<cumulativeWh>,<chk>\n` within a 200ms window.
- Poll cycle: all 3 nodes every 5 seconds, one retry on timeout/bad checksum before marking a node offline for that cycle.

HTTP (hub only): `GET /api/circuits` → JSON array of `{"id":N,"watts":F,"whToday":F,"online":bool}`.

## Setup & Deployment

1. Clamp each CT sensor around the live wire of the circuit it's monitoring (never around both live and neutral together, which cancels the reading to zero), and wire each node's voltage sensor to a source referencing the same mains phase.
2. Wire each node's MAX485 transceiver and the hub's MAX485, RTC, SD, and Ethernet shield; daisy-chain all 4 boards' A/B RS-485 lines with shared ground.
3. Install `SoftwareSerial`, `RTClib`, `SD`, and `Ethernet` (see `libraries.txt`).
4. Flash `src/meter-node/meter_node.ino` to each Nano, setting each one's `MY_ADDRESS` to 1, 2, or 3, and calibrate `CURRENT_CAL_FACTOR`/`VOLTAGE_CAL_FACTOR` against a known load and a multimeter.
5. Flash `src/hub/hub_controller.ino` to the Mega.
6. Power all 4 boards; confirm the hub's dashboard shows plausible live wattage for a known test load (e.g. a lamp) on each monitored circuit.

## Known Limitations & Path to Production

- **Safety note:** working around live mains wiring is dangerous — CT clamps are safe (they don't touch the conductor), but the voltage sensor and any other mains-adjacent wiring must only be done by someone qualified, with power off during installation.
- Cumulative watt-hours reset to zero on a node reboot — production metering needs this persisted (e.g. to EEPROM periodically, or reported cumulatively from the hub's own SD log instead of trusting node-side accumulation).
- No revenue-grade accuracy/calibration traceability — this is a monitoring aid, not a billing-grade meter.

## Extension Ideas

- Move cumulative Wh tracking to the hub (summing each poll's instantaneous power over the elapsed interval) so a node reboot doesn't lose historical totals.
- Add a cost-per-kWh setting to the dashboard for a simple running electricity-cost estimate.
- Scale to more circuits by extending the hub's poll list only, same as this tier's other RS-485 mesh projects.
