# Solar MPPT Battery Management System

An Uno Q tracks a solar panel's maximum power point with a Perturb & Observe algorithm driving a buck converter, coulomb-counts battery state of charge, logs historical power data to SD, and serves a local Ethernet dashboard with live and historical views.

## Board

**Arduino Uno Q** — two I2C current/voltage sensors, one PWM output to the buck converter, an SD module, and a W5500 Ethernet shield is a moderate, Uno-class pin budget; the Uno Q's role here mirrors this repo's other Ethernet-dashboard projects.

## Components

| Part | Qty |
|---|---|
| Arduino Uno Q | 1 |
| INA219 current/voltage sensor (I2C) — panel side | 1 |
| INA219 current/voltage sensor (I2C) — battery side | 1 |
| N-channel MOSFET buck converter stage (inductor, diode, MOSFET) | 1 |
| microSD card module (SPI) + microSD card | 1 |
| W5500 Ethernet shield/module (SPI) | 1 |
| 12V solar panel + lead-acid/LiFePO4 battery | 1 each |
| Jumper wires | ~15 |

## Architecture

Two INA219 sensors report panel and battery voltage/current every control tick; a Perturb & Observe loop nudges the buck converter's PWM duty cycle in the direction that increased delivered power last tick, tracking the panel's maximum power point as conditions change. Battery current is integrated over time (coulomb counting) to estimate state of charge. Every reading is logged to SD once a minute and served over a W5500-hosted JSON API plus a browser dashboard with a simple recent-history view. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| 20/21 (SDA/SCL) or A4/A5 | INA219 (panel) SDA/SCL, INA219 (battery) SDA/SCL | distinct I2C addresses (0x40, 0x41) via address pin strapping |
| D9 (PWM) | Buck converter MOSFET gate (via gate driver/resistor) | |
| 53 (SS) | SD module CS | |
| 49 | Ethernet (W5500) CS | |
| 50/51/52 | shared hardware SPI (SD + Ethernet) | |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

HTTP on port 80, trusted-LAN-only (no auth):

- `GET /api/data` → JSON: `{"panelV":F,"panelI":F,"panelW":F,"battV":F,"battI":F,"soc":F,"duty":N}`
- `GET /api/history` → JSON array of the last 50 logged SD rows, `[{"t":"...","panelW":F,"soc":F}, ...]`, for the dashboard's mini-chart.

## Setup & Deployment

1. Wire both INA219 sensors (confirm distinct I2C addresses), the buck converter stage, SD module, and W5500 shield as above.
2. Install `Adafruit INA219`, `Adafruit BusIO`, `SD`, and `Ethernet` (see `libraries.txt`).
3. Set `BATTERY_CAPACITY_AH` in the sketch to your battery's rated capacity, and set the initial `stateOfCharge` to a known value (e.g. fully charge the battery once, then set to 100).
4. Open `src/mppt_controller.ino`, set `ip` to a free LAN address, upload to the Uno Q.
5. Open Serial Monitor at 9600 baud to confirm the dashboard URL, then browse to it and confirm live panel/battery readings and the duty cycle update as sunlight changes.
6. Confirm `MPPTLOG.CSV` accumulates rows on the SD card roughly once a minute, and that `/api/history` reflects them.

## Known Limitations & Path to Production

- Coulomb counting drifts over time without periodic recalibration against a known full/empty state — production would add voltage-based SoC correction at the charge curve's flat/knee points.
- Perturb & Observe can lose track of the true MPP under rapidly changing (partial-shading) conditions — production trackers often use a hybrid P&O + incremental-conductance approach.
- No overcharge/over-discharge hard cutoff independent of software — a production BMS needs a hardware protection circuit as a backstop to firmware bugs.

## Extension Ideas

- Add a temperature sensor on the battery for temperature-compensated charge limits.
- Log irradiance (a simple LDR) alongside power to correlate MPPT efficiency with conditions.
- Add a low-SoC alert (buzzer or webhook) before the battery reaches a critical discharge level.
