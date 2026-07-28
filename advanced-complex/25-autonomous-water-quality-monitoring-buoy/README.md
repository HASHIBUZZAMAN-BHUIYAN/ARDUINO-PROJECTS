# Autonomous Water Quality Monitoring Buoy

A single-board flagship: an Uno Q fuses pH, turbidity, dissolved oxygen, water temperature, and water level with its own solar/battery power monitoring, runs an on-device anomaly engine that closes the loop on an aerator pump, keeps rolling statistics for historical trend analysis, and serves a token-authenticated REST API + dashboard for unattended, long-term deployment.

## Board

**Arduino Uno Q** — 5 water-quality/level sensors plus a power-monitoring sensor, SD, RTC, W5500 Ethernet, and a watchdog fit within an Uno-class pin budget with careful pin selection, matching this repo's convention of using Uno Q for networked/dashboard builds.

## Components

| Part | Qty |
|---|---|
| Arduino Uno Q | 1 |
| Analog pH sensor module | 1 |
| Analog turbidity sensor module | 1 |
| Analog dissolved-oxygen sensor module | 1 |
| DS18B20 water temperature probe (OneWire) | 1 |
| HC-SR04 ultrasonic sensor (water level) | 1 |
| INA219 current/voltage sensor (I2C, solar/battery monitoring) | 1 |
| Relay module (aerator pump + shore alarm) | 1 |
| DS3231 RTC module (I2C) | 1 |
| microSD card module (SPI) + microSD card | 1 |
| W5500 Ethernet shield/module (SPI, assumes a shore-side network bridge) | 1 |
| Jumper wires | ~20 |

## Architecture

All 5 water/level sensors plus the INA219 power monitor are read every cycle; an on-device anomaly engine compares each reading (and its rate of change, particularly for dissolved oxygen, where a fast drop can indicate an algal-bloom/fish-kill risk) against configured thresholds, and a confirmed anomaly closes the loop by engaging an aerator pump relay and a shore-side alarm buzzer. Rolling hourly min/max/avg statistics are computed on-device and logged to SD with an RTC timestamp, forming the historical-analytics trait, while a token-authenticated REST API + dashboard (assuming the buoy has a shore-relay/dock bridge providing Ethernet connectivity) exposes live readings, battery/solar status, and trend history. A software watchdog auto-recovers the board from any hang, essential for unattended long-term deployment. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| A0 | pH sensor analog out | |
| A1 | Turbidity sensor analog out | |
| A2 | Dissolved-oxygen sensor analog out | |
| D2 | DS18B20 (OneWire) | shared 4.7kΩ pull-up |
| D6 (trig), D7 (echo) | HC-SR04 (water level) | |
| A4/A5 | INA219 (I2C), DS3231 RTC (I2C) | |
| D8 | Aerator pump relay | |
| D9 | Shore alarm buzzer relay | |
| D10 (SS), D11/D12/D13 | SD module (SPI) | |
| D5 | Ethernet (W5500) CS | |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

HTTP on port 80, **token-authenticated** (`X-Auth-Token` header, same pattern as projects 22/23):

- `GET /api/state` → JSON: live pH, turbidity, DO, water temp, water level, battery/solar status, aerator/alarm state.
- `GET /api/history?range=24h` → JSON array of hourly rollups for the dashboard's trend chart.

## Setup & Deployment

1. Wire all 5 water-quality/level sensors, the INA219 power monitor, RTC, SD module, relay module, and W5500 shield as above.
2. Install `OneWire`, `DallasTemperature`, `Adafruit INA219` + `Adafruit BusIO`, `RTClib`, `SD`, and `Ethernet` (see `libraries.txt`).
3. Open `src/water_quality_buoy.ino`, set `API_TOKEN` to a real secret and calibrate the pH/turbidity/DO sensors' analog-to-value conversion constants against known reference solutions.
4. The first upload only, uncomment the `rtc.adjust(...)` line to set the clock, then re-comment and re-upload.
5. Upload to the Uno Q; deploy in the water body with the Ethernet link connected to a shore-side network bridge.
6. Confirm the dashboard shows plausible live readings and that a simulated DO-drop (e.g. temporarily adjusting the threshold for a bench test) engages the aerator relay and shore alarm.
7. Confirm hourly rollup rows accumulate on SD and are visible via `/api/history`.

## Known Limitations & Path to Production

- Analog water-quality sensors (pH, turbidity, DO) drift over weeks/months and need periodic recalibration against reference solutions — production deployment would schedule regular manual recalibration or use self-calibrating probes.
- The Ethernet-based dashboard assumes a shore-side network bridge is always available; a fully autonomous open-water buoy would more realistically use a cellular or satellite uplink (see project 24's SIM800L pattern) instead of wired/local Ethernet.
- No biofouling mitigation (algae/growth on submerged sensor surfaces skews readings over time) — production sensors in this application typically need periodic cleaning or anti-fouling coatings.

## Extension Ideas

- Replace the Ethernet uplink with a cellular module (reusing project 24's store-and-forward AT-command pattern) for true open-water deployment.
- Add a self-cleaning wiper mechanism for the submerged optical sensors (turbidity/DO) to reduce biofouling drift.
- Add multi-day trend-based alerting (e.g. "DO has been declining for 3 consecutive days") on top of the existing instantaneous-threshold anomaly engine.
