# RS-485 Sensor Mesh with Uno Q Hub

Three Arduino Uno field nodes (climate, air quality, occupancy) report over a shared RS-485 bus to an Uno Q hub, which polls each node in turn, logs every reading to SD with RTC timestamps, and serves a local Ethernet dashboard summarizing all three nodes' latest status.

## Board(s) & Roles

- **3x Arduino Uno** — field nodes, each wired to a different sensor pair and a MAX485 transceiver: **Node 1 "Climate"** (DHT22 + BH1750 light), **Node 2 "Air Quality"** (MQ-135 gas + analog sound sensor), **Node 3 "Occupancy"** (PIR + door reed switch).
- **Arduino Uno Q** — bus master/hub: polls all 3 nodes over RS-485, logs to SD with an RTC timestamp, and serves an Ethernet dashboard. Fitted with a MAX485 transceiver, a W5500 Ethernet shield, an SD module, and a DS3231 RTC.

## Components

| Part | Qty |
|---|---|
| Arduino Uno (field node) | 3 |
| Arduino Uno Q (hub) | 1 |
| MAX485 RS-485 transceiver module | 4 |
| DHT22 temperature/humidity sensor | 1 |
| BH1750 light sensor (I2C) | 1 |
| MQ-135 air quality sensor (analog) | 1 |
| Sound level sensor module (analog) | 1 |
| HC-SR501 PIR motion sensor | 1 |
| Magnetic reed switch | 1 |
| DS3231 RTC module (I2C, hub only) | 1 |
| microSD card module (SPI, hub only) + microSD card | 1 |
| W5500 Ethernet shield/module (SPI, hub only) | 1 |
| Twisted-pair cable (RS-485 bus, A/B + GND) | as needed |
| Jumper wires | ~40 |

## Architecture

The hub cycles through node addresses 1-3 every 10 seconds, sending a `POLL` request over the shared RS-485 bus; whichever node matches the address replies with its sensor readings within a timeout window. The hub logs every successful reply (with an RTC timestamp) to SD, and marks a node "OFFLINE" for a cycle if it fails to reply after one retry. A W5500-hosted dashboard summarizes all 3 nodes' most recent readings and online/offline status. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

### Field Nodes (Uno x3)

| Arduino Pin | Component | Notes |
|---|---|---|
| D2 | MAX485 DE+RE (tied together) | transmit-enable |
| D8, D9 | MAX485 RO, DI | to SoftwareSerial RX/TX |
| A/B | MAX485 A/B | to shared RS-485 bus, daisy-chained across all 4 boards |
| (Node 1) D3, A4/A5 | DHT22 DATA, BH1750 SDA/SCL | |
| (Node 2) A0, A1 | MQ-135 AOUT, sound sensor AOUT | |
| (Node 3) D4, D5 | PIR OUT, reed switch (`INPUT_PULLUP`) | |

### Hub (Uno Q)

| Arduino Pin | Component | Notes |
|---|---|---|
| D2 | MAX485 DE+RE | |
| D8, D9 | MAX485 RO, DI | SoftwareSerial |
| A4/A5 | DS3231 RTC (I2C) | Uno Q's I2C pins (Uno-class pinout) |
| D4 | SD module CS | |
| D7 | Ethernet (W5500) CS | |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

RS-485 half-duplex, 9600 baud via `SoftwareSerial`, DE/RE tied to one digital pin per board for transmit-enable:

- Hub → node: `POLL <addr>\n` (addr 1, 2, or 3).
- Node → hub: `D,<addr>,<value1>,<value2>,<seq>,<chk>\n` within a 200ms window, where `<chk>` is a sum-mod-256 checksum of the preceding fields.
- The hub retries once on timeout or bad checksum, then marks that node `OFFLINE` for the current cycle.
- Poll cycle: all 3 nodes every 10 seconds.

HTTP (hub only), port 80, trusted-LAN-only: `GET /api/nodes` → JSON array of `{"addr":N,"online":bool,"v1":F,"v2":F,"lastSeen":"..."}`.

## Setup & Deployment

1. Wire each field node's sensors and MAX485 transceiver; wire the hub's MAX485, RTC, SD, and Ethernet shield. Daisy-chain all 4 boards' A/B RS-485 lines together with a shared ground.
2. Install `SoftwareSerial` (built-in), `DHT sensor library`, `BH1750`, `RTClib`, `SD`, and `Ethernet` (see `libraries.txt`).
3. Flash `src/node-climate/node_climate.ino` to Node 1's Uno, `src/node-air-quality/node_air_quality.ino` to Node 2's Uno, `src/node-occupancy/node_occupancy.ino` to Node 3's Uno — confirm each node's `MY_ADDRESS` constant matches its intended address (1, 2, 3).
4. Flash `src/hub/hub_controller.ino` to the Uno Q.
5. Power all 4 boards. Open the hub's Serial Monitor at 9600 baud and confirm all 3 nodes report `online` within the first couple of poll cycles.
6. Browse to the hub's printed dashboard URL and confirm all 3 nodes' latest readings display and update every 10 seconds.
7. Power off one node and confirm the dashboard shows it `OFFLINE` after its next poll cycle.

## Known Limitations & Path to Production

- No collision detection beyond the hub's own poll/response turn-taking — this works because only the hub-addressed node ever replies, but a rogue/misconfigured node could still jam the bus.
- SoftwareSerial-based RS-485 tops out around 9600-19200 baud reliably on these boards; a production deployment with more nodes or faster polling would benefit from hardware-UART RS-485 boards.
- No bus termination resistors specified — long real-world runs need 120Ω termination at both bus ends to avoid reflections.

## Extension Ideas

- Add more field nodes (each a distinct address) without any hub code changes beyond extending the poll list.
- Add per-node battery-voltage reporting so the hub can flag "node online but low battery" distinctly from "offline."
- Move the bus protocol to a lightweight Modbus-RTU implementation for interoperability with off-the-shelf RS-485 sensors.
