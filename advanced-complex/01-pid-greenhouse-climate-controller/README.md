# PID Greenhouse Climate Controller

A single Mega runs independent PID control loops for greenhouse temperature and humidity, schedules grow-lights off an RTC, logs every actuator/sensor state to SD, and serves a local Ethernet dashboard for live monitoring and setpoint changes.

## Board

**Arduino Mega 2560** — the actuator count (heater relay, fan relay, mister relay, grow-light relay), sensor count (DHT22, light, soil moisture), SD card (SPI), W5500 Ethernet shield (SPI, separate CS), and DS3231 RTC (I2C) together need more usable pins and a second SPI CS line than an Uno comfortably offers.

## Components

| Part | Qty |
|---|---|
| Arduino Mega 2560 | 1 |
| DHT22 temperature/humidity sensor | 1 |
| BH1750 light sensor (I2C) | 1 |
| Capacitive soil moisture probe (analog) | 1 |
| DS3231 RTC module (I2C) | 1 |
| microSD card module (SPI) + microSD card | 1 |
| W5500 Ethernet shield/module (SPI) | 1 |
| 4-channel relay module (heater, fan, mister, grow-light) | 1 |
| Jumper wires | ~30 |

## Architecture

DHT22 + BH1750 + soil probe feed two independent PID loops (temperature→heater/fan, humidity→mister) that run every control tick; the RTC gates the grow-light relay on a daily schedule. Every reading, PID output, and relay state is appended to an SD CSV once a minute, and a W5500-hosted HTTP server exposes the latest snapshot as JSON plus a setpoint-update endpoint. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

| Arduino Mega Pin | Component | Notes |
|---|---|---|
| D2 | DHT22 DATA | |
| 20 (SDA) | BH1750 SDA, DS3231 SDA | Mega's dedicated I2C pins |
| 21 (SCL) | BH1750 SCL, DS3231 SCL | |
| A0 | Soil moisture probe AOUT | |
| D3 | Heater relay IN | |
| D4 | Fan relay IN | |
| D5 | Mister relay IN | |
| D6 | Grow-light relay IN | |
| 53 (SS) | SD module CS | dedicated CS, Mega hardware SPI |
| 49 | Ethernet (W5500) CS | second CS on same SPI bus |
| 50/51/52 | MISO/MOSI/SCK | shared hardware SPI bus (SD + Ethernet) |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

HTTP on port 80 over the W5500 shield, trusted-LAN-only (no auth — same tradeoff as this repo's existing `advanced/03-ethernet-home-automation-server`):

- `GET /api/data` → JSON: `{"temp":F,"hum":F,"light":F,"soil":F,"heater":0|1,"fan":0|1,"mist":0|1,"light_relay":0|1,"setpointTemp":F,"setpointHum":F}`
- `GET /setpoint?t=NN&h=NN` → updates the temperature/humidity PID setpoints, returns `200 OK`

## Setup & Deployment

1. Wire all sensors, relays, the SD module, and the W5500 shield per the table above — SD and Ethernet share the SPI bus but use separate CS pins (53 and 49).
2. Install libraries from `libraries.txt`.
3. Format the microSD card FAT32 and insert it.
4. Open `src/greenhouse_controller.ino`, set `ip` to a free address on your LAN, and — the first upload only — uncomment the `rtc.adjust(...)` line to set the clock, then re-comment and re-upload.
5. Select **Tools > Board > Arduino Mega or Mega 2560**, upload.
6. Open Serial Monitor at 9600 baud to confirm the printed dashboard URL, then browse to it and confirm live readings update and `/setpoint` changes take effect.
7. Confirm `GREENHOUSE.CSV` appears on the SD card with rows appended roughly once a minute.

## Known Limitations & Path to Production

- No authentication on the HTTP API — anyone on the LAN can change setpoints. Production would need at minimum a shared-secret header check (see project 22/23/25 for the token-auth pattern used elsewhere in this tier) and ideally TLS via an offloading reverse proxy, since the W5500 has no native TLS.
- PID loops use fixed, hand-tuned gains — no auto-tuning or anti-windup beyond simple output clamping.
- Single point of failure: if the Mega resets, PID state and setpoints (kept in RAM) revert to compiled defaults. Production would persist setpoints to EEPROM and reload them at boot.

## Extension Ideas

- Add EEPROM-persisted setpoints so they survive a reset.
- Add a second grow-light zone with its own RTC schedule.
- Push CSV rows to a cloud time-series service (e.g. via an ESP8266 AT co-processor, as in `advanced/07`) in addition to local SD logging.
