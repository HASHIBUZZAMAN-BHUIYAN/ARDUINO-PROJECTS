# Home Energy Resilience Management System

A single-board flagship: a Mega monitors grid, solar, and battery power legs via three INA219 sensors, automatically prefers solar over battery over grid and sheds non-critical loads as battery state-of-charge drops, persists critical state through power loss, logs historical rollups to SD, and exposes a token-authenticated REST API + dashboard.

## Board

**Arduino Mega 2560** — 3 I2C power sensors, a 4-relay transfer/load-shedding bank, EEPROM-persisted state, SD logging, and Ethernet together need the Mega's I2C and pin headroom.

## Components

| Part | Qty |
|---|---|
| Arduino Mega 2560 | 1 |
| INA219 current/voltage sensor (I2C) — grid leg | 1 |
| INA219 current/voltage sensor (I2C) — solar leg | 1 |
| INA219 current/voltage sensor (I2C) — battery leg | 1 |
| 4-channel relay module (transfer switch + load priority shedding) | 1 |
| DS3231 RTC module (I2C) | 1 |
| microSD card module (SPI) + microSD card | 1 |
| W5500 Ethernet shield/module (SPI) | 1 |
| Jumper wires | ~25 |

## Architecture

Three INA219 sensors continuously report grid, solar, and battery power; a decision engine prefers solar when available, falls back to battery, and only engages grid import when both are insufficient, while a separate load-shedding ladder disconnects non-critical circuits (via 3 of the 4 relay channels, in priority order) as battery state-of-charge drops below configured thresholds — a closed-loop automation layer, not just monitoring. Every relay-state and Wh-counter change is written to EEPROM immediately, so a power interruption mid-operation resumes correctly rather than reverting to defaults. SD logging computes daily/weekly min/max/avg rollups, and a token-authenticated REST API + dashboard exposes live power flow and historical rollups. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

| Arduino Mega Pin | Component | Notes |
|---|---|---|
| 20/21 (SDA/SCL) | 3x INA219 (grid, solar, battery; addresses 0x40/0x41/0x44), DS3231 RTC | Mega's dedicated I2C pins |
| D30, D31 | Transfer relay (grid-in, solar/battery-in) | mutually exclusive |
| D32, D33 | Load-shedding relays (priority 1, priority 2 non-critical circuits) | |
| 53/51/50/52 | SD CS/MOSI/MISO/SCK | hardware SPI |
| 49 | Ethernet (W5500) CS | shares SPI bus with SD |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

HTTP on port 80, **token-authenticated** (`X-Auth-Token` header, same pattern as project 22):

- `GET /api/state` → JSON: grid/solar/battery watts, SoC%, active source, shed-load states.
- `GET /api/history?range=7d` → JSON array of daily rollups (min/max/avg power per source).

## Setup & Deployment

1. Wire all 3 INA219 sensors on the shared I2C bus (distinct addresses), the transfer/shedding relay bank, RTC, SD module, and W5500 shield.
2. Install `Adafruit INA219` + `Adafruit BusIO`, `RTClib`, `SD`, and `Ethernet` (see `libraries.txt`).
3. Open `src/energy_resilience.ino`, set `API_TOKEN` to a real secret and `ip` to a free LAN address.
4. The first upload only, uncomment the `rtc.adjust(...)` line to set the clock, then re-comment and re-upload.
5. Upload to the Mega; confirm on first boot it initializes EEPROM state to safe defaults (grid active, no loads shed).
6. Simulate a low-battery condition (or adjust `BATTERY_LOW_THRESHOLD` for a bench test) and confirm load-shedding relays engage in priority order as SoC drops, and disengage in reverse order as it recovers.
7. Power-cycle the Mega mid-operation and confirm relay states and Wh counters resume from EEPROM rather than resetting.

## Known Limitations & Path to Production

- Source-selection logic assumes clean, glitch-free power sensing — a production automatic transfer switch needs additional debounce/hysteresis and, critically, a hardware-level interlock so the transfer relay can never physically connect two sources simultaneously (a software bug alone should not be able to cause that).
- EEPROM has a finite write-cycle lifetime; frequent relay-state changes in a noisy environment could wear it out over years — production would rate-limit EEPROM writes or use FRAM instead.
- No AFCI/GFCI-equivalent safety monitoring — this is an energy-flow manager, not a substitute for proper electrical safety equipment.

## Extension Ideas

- Add a generator-start relay output for a 3-source (grid/solar+battery/generator) hierarchy.
- Add weather-forecast-aware pre-emptive load shedding (shed proactively ahead of a predicted low-solar day).
- Migrate EEPROM state persistence to an external FRAM module for higher write-cycle endurance.
