# Multiplexed 8-Zone Soil Irrigation Scheduler

A Mega multiplexes 8 soil-moisture probes through a single analog pin, runs an RTC-based, rain-adaptive watering schedule with closed-loop moisture-threshold shutoff per zone, logs every event to SD, and exposes a REST API + dashboard for manual override.

## Board

**Arduino Mega 2560** — 8 solenoid valve relay outputs, an analog multiplexer's 4 select lines, a rain sensor, an RTC, an SD module, and a W5500 shield add up to well beyond an Uno's comfortable pin budget.

## Components

| Part | Qty |
|---|---|
| Arduino Mega 2560 | 1 |
| CD74HC4067 16-channel analog multiplexer | 1 |
| Capacitive soil moisture probe | 8 |
| Digital rain sensor module | 1 |
| DS3231 RTC module (I2C) | 1 |
| 8-channel relay module (solenoid valves) | 1 |
| Solenoid valve | 8 |
| microSD card module (SPI) + microSD card | 1 |
| W5500 Ethernet shield/module (SPI) | 1 |
| Jumper wires | ~40 |

## Architecture

The CD74HC4067 multiplexer routes one of 8 soil probes onto a single Mega analog pin at a time, selected via 4 digital lines, so the whole zone array uses only 5 pins instead of 8 separate analog inputs. Each zone has an RTC-scheduled watering window (suppressed if the rain sensor reports recent rainfall); when a window opens, the corresponding valve relay opens until the zone's multiplexed soil reading crosses a target threshold or a max-runtime safety timeout elapses, whichever comes first — a genuine closed loop rather than a fixed-duration timer. Every watering event and periodic soil reading is logged to SD, and a W5500-hosted REST API + dashboard exposes live zone status and manual on/off override. See [architecture.md](architecture.md) for the full diagram and data-flow walkthrough.

## Wiring

| Arduino Mega Pin | Component | Notes |
|---|---|---|
| A0 | CD74HC4067 SIG (common analog out) | |
| D22, D23, D24, D25 | CD74HC4067 S0-S3 (channel select) | selects which of 8 wired probes is read |
| D2 | Rain sensor digital out | active LOW = rain detected |
| 20/21 (SDA/SCL) | DS3231 RTC | |
| D30-D37 | 8-channel relay module IN1-IN8 | one per solenoid valve |
| 53/51/50/52 | SD CS/MOSI/MISO/SCK | |
| 49 | Ethernet (W5500) CS | shares SPI bus with SD |

See [wiring/wiring.md](wiring/wiring.md) for the full ASCII diagram.

## Networking & Protocol

HTTP on port 80, trusted-LAN-only (no auth):

- `GET /api/zones` → JSON array, one object per zone: `{"zone":N,"soil":F,"valveOpen":0|1,"scheduled":"HH:MM","lastWatered":"..."}`
- `POST /api/zones/<n>/on` and `POST /api/zones/<n>/off` → manual override, bypassing the schedule until the next scheduled window.

## Setup & Deployment

1. Wire all 8 probes to the multiplexer's 8 input channels, the multiplexer's SIG/select lines to the Mega, the rain sensor, RTC, relay module (one channel per valve), SD module, and W5500 shield as above.
2. Install `RTClib`, `SD`, and `Ethernet` (see `libraries.txt`).
3. Set each zone's schedule window and moisture threshold in the `zones[]` array in the sketch.
4. Open `src/irrigation_scheduler.ino`; the first upload only, uncomment the `rtc.adjust(...)` line to set the clock, then re-comment and re-upload.
5. Upload to the Mega, open Serial Monitor at 9600 baud to confirm the dashboard URL.
6. Manually trigger a zone via `POST /api/zones/0/on` (e.g. with `curl`) and confirm the corresponding valve relay energizes and de-energizes correctly at the moisture threshold or timeout.
7. Confirm `IRRIGLOG.CSV` accumulates watering events on the SD card.

## Known Limitations & Path to Production

- A single shared analog pin means all 8 zones are sampled sequentially, not simultaneously — fine for slow-changing soil moisture, unsuitable if faster-changing signals were multiplexed the same way.
- No flow-sensor confirmation that a valve physically opened (e.g. a stuck/broken solenoid would go undetected) — production would add an inline flow sensor per zone or per manifold.
- No authentication on the override API.

## Extension Ideas

- Add a flow sensor on the main supply line to detect a stuck-open valve as a safety fault (shut all valves + alert).
- Add weather-forecast-based scheduling (skip if a forecast API predicts rain) instead of only reactive rain sensing.
- Chain a second CD74HC4067 to scale to 16 zones without adding more analog pins.
