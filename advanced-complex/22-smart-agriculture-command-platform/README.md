# Smart Agriculture Command Platform

A production-grade, single-board flagship: a Mega multiplexes 8 soil-moisture zones and fuses climate sensors into unified irrigation + climate PID control, keeps a structured SD "database" queryable over serial, and exposes a token-authenticated REST API plus a live dashboard over Ethernet — the most complete build in this tier.

## Board

**Arduino Mega 2560** — 8 multiplexed soil zones, climate sensing, 2 PID control domains (irrigation + climate), an SD "database," Ethernet, and a watchdog together need the Mega's full pin and interrupt budget.

## Components

| Part | Qty |
|---|---|
| Arduino Mega 2560 | 1 |
| CD74HC4067 16-channel analog multiplexer | 1 |
| Capacitive soil moisture probe | 8 |
| BME280 (I2C: temperature/humidity/pressure) | 1 |
| BH1750 light sensor (I2C) | 1 |
| DS3231 RTC module (I2C) | 1 |
| 8-channel relay module (solenoid valves) | 1 |
| 4-channel relay module (heater, fan, mister, grow-light) | 1 |
| microSD card module (SPI) + microSD card | 1 |
| W5500 Ethernet shield/module (SPI) | 1 |
| Jumper wires | ~45 |

## Architecture

Eight soil-moisture zones are multiplexed onto one analog pin (as in this tier's project 06) and each independently closed-loop-irrigated against a moisture threshold with a max-runtime safety timeout; a separate climate PID pair (temperature→heater/fan, humidity→mister) and an RTC-scheduled grow-light relay run alongside, sharing the same control core. Every irrigation event, climate reading, and PID output is written as a structured CSV row to SD, which doubles as a lightweight flat-file database queryable over serial with a small `QUERY` command language. A W5500-hosted REST API requires a shared-secret token on every request (`X-Auth-Token` header) and serves both live state and historical rollups to a browser dashboard; a hardware watchdog (`<avr/wdt.h>`) auto-reboots the board if the main loop ever hangs. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

| Arduino Mega Pin | Component | Notes |
|---|---|---|
| A0 | CD74HC4067 SIG | multiplexed soil-zone input |
| D22, D23, D24, D25 | CD74HC4067 S0-S3 | channel select |
| 20/21 (SDA/SCL) | BME280, BH1750, DS3231 RTC | Mega's dedicated I2C pins |
| D30-D37 | 8-ch relay (irrigation valves) | |
| D38, D39, D40, D41 | 4-ch relay (heater, fan, mister, grow-light) | |
| 53/51/50/52 | SD CS/MOSI/MISO/SCK | hardware SPI |
| 49 | Ethernet (W5500) CS | shares SPI bus with SD |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

HTTP on port 80, **token-authenticated**: every request must include header `X-Auth-Token: <token>` matching the sketch's compiled-in `API_TOKEN` constant (a placeholder value — replace before any real deployment), or the server responds `401 Unauthorized`.

- `GET /api/state` → JSON: full current snapshot (all 8 zones' soil readings/valve states, climate readings, actuator states).
- `GET /api/history?range=24h` → JSON array of recent logged rows for the dashboard's charts.
- `POST /api/zones/<n>/on` / `/off` → manual zone override.
- `POST /api/climate/setpoint?t=NN&h=NN` → updates the climate PID setpoints.

Serial "database" query language (9600 baud, local console only, no auth needed since it requires physical/USB access): `QUERY ZONE <n> LAST <hours>H` scans the SD CSV and prints matching rows.

## Setup & Deployment

1. Wire the multiplexer + 8 soil probes, climate sensors, both relay modules, SD module, and Ethernet shield as above.
2. Install `Adafruit BME280` + `Adafruit BusIO`, `BH1750`, `RTClib`, `SD`, and `Ethernet` (see `libraries.txt`).
3. Open `src/agriculture_platform.ino`, set `API_TOKEN` to a real secret value (never commit the real one to a public repo) and set `ip` to a free LAN address.
4. The first upload only, uncomment the `rtc.adjust(...)` line to set the clock, then re-comment and re-upload.
5. Upload to the Mega, open Serial Monitor at 9600 baud to confirm the dashboard URL and try a `QUERY ZONE 0 LAST 24H` command.
6. Browse to the dashboard, enter the configured token when prompted, and confirm live zone/climate data plus historical charts render.
7. Confirm a deliberately wrong token on `/api/state` returns `401` (e.g. via `curl -H "X-Auth-Token: wrong" http://<ip>/api/state`).
8. Simulate a hang (e.g. an infinite loop temporarily added for testing) and confirm the watchdog resets the board within its configured timeout — remove the test code afterward.

## Known Limitations & Path to Production

- The API token is a single shared secret compiled into firmware, not per-user credentials or rotated tokens — production would use a proper auth scheme (e.g. per-device tokens issued from a backend) and TLS (not available on this AVR+W5500 stack without an offloading proxy).
- The SD "database" is a flat CSV scanned linearly per query — fine at this data volume, but would need real indexing or a purpose-built time-series database at larger scale/longer retention.
- Watchdog recovery restarts the whole sketch from `setup()`, losing in-RAM PID integrator state — recovery is safe but not seamless.

## Extension Ideas

- Rotate the API token periodically and require re-entry on the dashboard.
- Add outbound webhook alerts (e.g. via an ESP8266 AT co-processor, see `advanced/07`) for critical events like a stuck valve or an extended climate excursion.
- Add per-zone historical yield/watering-efficiency reporting computed from the SD log.
