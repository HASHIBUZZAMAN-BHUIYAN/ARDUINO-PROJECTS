# Cold Chain Logistics Monitor

A single-board flagship for shipping-container/reefer-truck monitoring: a Mega fuses multi-point temperature, humidity, shock, door, and tamper sensing, buffers readings to SD when offline, and pushes authenticated alerts and periodic snapshots over a SIM800L cellular module using a store-and-forward pattern so no data is lost during a connectivity gap.

## Board

**Arduino Mega 2560** — multiple OneWire temperature probes, a DHT22, an accelerometer, two switches, an SD module, and a SIM800L cellular module together need more pins/UARTs than an Uno offers.

## Components

| Part | Qty |
|---|---|
| Arduino Mega 2560 | 1 |
| DS18B20 temperature probe (OneWire, multiple points in the load) | 3 |
| DHT22 humidity sensor | 1 |
| ADXL345 accelerometer (I2C, shock/impact detection) | 1 |
| Magnetic reed switch (door) | 1 |
| Tamper switch (enclosure) | 1 |
| SIM800L GPRS/cellular module | 1 |
| Relay module (auxiliary cooling activation) | 1 |
| microSD card module (SPI) + microSD card | 1 |
| DS3231 RTC module (I2C) | 1 |
| Jumper wires | ~25 |

## Architecture

Three DS18B20 probes at different points in the load, a DHT22, and an ADXL345 accelerometer are fused every reading cycle; a shock event, a temperature excursion, a door-open, or a tamper trip are all logged as timestamped events to SD immediately. If cellular connectivity is available, each buffered SD row is sent as an authenticated HTTP POST over the SIM800L (AT commands via `SoftwareSerial`) to a configured ingest endpoint and marked "synced" only after a server acknowledgment; if connectivity is down, rows simply accumulate on SD and are retried in order once connectivity returns — a genuine store-and-forward reliability pattern. A sustained temperature excursion also closes a local loop: it engages an auxiliary cooling relay and sounds a buzzer, independent of whether the alert successfully reaches the cloud. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

| Arduino Mega Pin | Component | Notes |
|---|---|---|
| D2 | 3x DS18B20 (OneWire bus, each with a unique ROM address) | shared 4.7kΩ pull-up |
| D3 | DHT22 DATA | |
| 20/21 (SDA/SCL) | ADXL345, DS3231 RTC | |
| D4 | Door reed switch | `INPUT_PULLUP` |
| D5 | Tamper switch | `INPUT_PULLUP` |
| D6 | Auxiliary cooling relay | |
| D7 | Buzzer | |
| D16 (TX2), D17 (RX2) | SIM800L (`Serial2`) | |
| 53/51/50/52 | SD CS/MOSI/MISO/SCK | hardware SPI |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

Cellular (SIM800L, AT commands over `Serial2`), HTTP POST with a bearer token, store-and-forward:

- Each buffered SD row becomes a JSON payload: `{"ts":"...","t1":F,"t2":F,"t3":F,"hum":F,"shockG":F,"door":0|1,"tamper":0|1}`.
- Sent via SIM800L's `AT+HTTPPARA`/`AT+HTTPACTION` command sequence with header `Authorization: Bearer <token>`, to a configured ingest URL.
- A row is marked synced in the SD log only after a `200`-class HTTP response is confirmed; anything else leaves it pending for the next sync attempt.

```cpp
// ---- credentials placeholder — replace before use, never commit real secrets ----
const char* APN = "YOUR_CARRIER_APN";
const char* INGEST_URL = "http://your-ingest-endpoint.example.com/coldchain";
const char* BEARER_TOKEN = "YOUR_BEARER_TOKEN";
// ---------------------------------------------------------------------------------
```

## Setup & Deployment

1. Wire the 3 DS18B20 probes (record each one's unique 64-bit ROM address for `src/cold_chain_monitor.ino`), DHT22, ADXL345, door/tamper switches, cooling relay, buzzer, RTC, SD module, and SIM800L module.
2. Install `OneWire`, `DallasTemperature`, `DHT sensor library`, `Adafruit ADXL345` + `Adafruit Sensor`, `RTClib`, and `SD` (see `libraries.txt`); the SIM800L side uses raw AT commands with no additional library.
3. Insert a SIM card with data service into the SIM800L module and edit the credentials placeholder block with your real APN, ingest URL, and bearer token.
4. Open `src/cold_chain_monitor.ino`, update each DS18B20's ROM address constant to match your specific probes (found via a one-time `OneWire` address-scan sketch).
5. Upload to the Mega; confirm via Serial Monitor that the SIM800L registers on the cellular network (`AT+CREG?` returns registered).
6. Confirm normal readings sync to the ingest endpoint periodically, and that simulated events (door open, tap the accelerometer, cover the tamper switch) log immediately and sync as separate rows.
7. Disconnect the SIM800L's antenna (or otherwise force a connectivity loss) and confirm rows continue to accumulate on SD, then reconnect and confirm all pending rows sync in order.

## Known Limitations & Path to Production

- The bearer token is a single static secret compiled into firmware, not rotated or device-specific — production would issue unique per-device credentials from a backend and support rotation.
- SIM800L's plain-HTTP AT command flow (as used here for simplicity/portability across firmware versions) does not provide TLS; a production deployment needs a SIM7000-class module with HTTPS support, or a TLS-terminating gateway between the device and the ingest endpoint.
- Local closed-loop cooling response has no independent hardware backstop — a firmware hang would leave the auxiliary cooling relay in whatever state it was last set, with no watchdog in this build (see project 22 for the watchdog pattern to combine with this one in production).

## Extension Ideas

- Add a watchdog timer (see project 22) for autonomous recovery from a firmware hang mid-transit.
- Add GPS (via a UART GPS module) to attach location to every logged event.
- Migrate to a TLS-capable cellular module and per-device credentials for a production rollout.
