# Multi-Sensor Weather Buoy Datalogger

An Uno Q fuses six weather/water sensors — anemometer, wind vane, tipping-bucket rain gauge, BME280, UV sensor, and an ultrasonic water-level sensor — into a timestamped SD log with on-device daily min/max/avg rollups, and serves a local Ethernet dashboard with a recent-history sparkline.

## Board

**Arduino Uno Q** — six sensors including two interrupt-driven pulse counters, an RTC, an SD module, and a W5500 shield fit within an Uno-class pin budget with careful pin selection; the Uno Q's role here matches its use elsewhere in this repo for Ethernet-dashboard projects.

## Components

| Part | Qty |
|---|---|
| Arduino Uno Q | 1 |
| Cup anemometer (reed-switch pulse output) | 1 |
| Wind vane (analog voltage-divider output) | 1 |
| Tipping-bucket rain gauge (reed-switch pulse output) | 1 |
| BME280 (I2C: temperature/humidity/pressure) | 1 |
| Analog UV index sensor | 1 |
| HC-SR04 ultrasonic sensor (water/tide level) | 1 |
| DS3231 RTC module (I2C) | 1 |
| microSD card module (SPI) + microSD card | 1 |
| W5500 Ethernet shield/module (SPI) | 1 |
| Jumper wires | ~20 |

## Architecture

The anemometer and rain gauge are wired to interrupt pins and counted as pulses since boot; every 5 minutes, pulse counts are converted to wind speed and rainfall totals, combined with the wind vane, BME280, and UV readings into one row, timestamped via RTC, and appended to SD — while running totals also feed min/max/avg accumulators that get flushed into a daily summary row at local midnight. A W5500-hosted dashboard shows current conditions plus a sparkline built from the last 24 logged rows re-read from SD. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| D2 (INT) | Anemometer reed switch | pulse-counted, hardware interrupt |
| D3 (INT) | Rain gauge reed switch | pulse-counted, hardware interrupt |
| A0 | Wind vane voltage divider | mapped to 8 compass directions |
| A1 | UV sensor analog out | |
| D6 (trig), D7 (echo) | HC-SR04 (water level) | |
| A4/A5 | BME280, DS3231 RTC (I2C) | |
| D10 (SS), D11/D12/D13 | SD module (SPI) | |
| D9 | Ethernet (W5500) CS | shares SPI bus |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

HTTP on port 80, trusted-LAN-only (no auth):

- `GET /api/data` → JSON current conditions: `{"windKph":F,"windDir":"NE","rainMmToday":F,"temp":F,"hum":F,"pressure":F,"uv":F,"waterCm":F}`
- `GET /api/history` → JSON array of the last 24 logged rows, for the dashboard sparkline.

## Setup & Deployment

1. Wire all six sensors, the RTC, SD module, and W5500 shield as above; mount the anemometer/wind vane/rain gauge outdoors per their manufacturer instructions and run weatherproofed cabling back to the enclosure.
2. Install `Adafruit BME280` + `Adafruit BusIO`, `RTClib`, `SD`, and `Ethernet` (see `libraries.txt`).
3. Calibrate `ANEMOMETER_KPH_PER_PULSE_HZ` and `RAIN_MM_PER_TIP` constants in the sketch against your specific hardware's datasheet.
4. Open `src/weather_buoy.ino`; the first upload only, uncomment the `rtc.adjust(...)` line to set the clock, then re-comment and re-upload.
5. Upload to the Uno Q, open Serial Monitor at 9600 baud to confirm the dashboard URL.
6. Confirm `WEATHER.CSV` accumulates rows every 5 minutes and that a daily summary row is appended at local midnight.

## Known Limitations & Path to Production

- No lightning/surge protection on the outdoor sensor wiring — a production buoy/weather-station needs surge-protected cabling given lightning exposure.
- Wind gust (peak within an interval) is not separately tracked from average wind speed — a production station would log both.
- Single point of failure: no backup power, so an outage during a storm loses live data (though prior SD rows remain intact).

## Extension Ideas

- Add battery + solar charging with a coulomb-counted power log alongside weather data (see project 05's MPPT/battery pattern).
- Add a lightning detector module (e.g. AS3935) as a seventh fused sensor.
- Push daily summaries to a cloud weather-sharing service via an ESP8266 AT co-processor (see `advanced/07`).
