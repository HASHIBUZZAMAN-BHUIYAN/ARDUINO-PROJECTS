# GPS Data Logging Tracker

A GPS module feeds position/speed data to the board, which timestamps each fix using an onboard RTC as a backup clock source and appends a track log (CSV: time, latitude, longitude, speed, altitude) to an SD card — useful for hikes, bike rides, or tracking a model vehicle.

## Difficulty & Board

**Tier:** Advanced
**Board:** Arduino Uno

Reasoning: a GPS module (SoftwareSerial, 2 pins), an SD card module (hardware SPI, 4 pins), an RTC (I2C, 2 pins), and a status LED is a moderate pin count that fits comfortably on an Uno — no motors or dense sensor arrays are involved, so the extra Mega headroom isn't needed here, and using the Uno keeps the board rotation balanced.

## Components

| Part | Qty |
|---|---|
| Arduino Uno | 1 |
| NEO-6M GPS module (UART) | 1 |
| microSD card module (SPI) + microSD card | 1 |
| DS3231 RTC module (I2C) | 1 |
| LED (fix status indicator) | 1 |
| 220 Ω resistor | 1 |
| Battery pack (for portable use) | 1 |
| Breadboard | 1 |
| Jumper wires | ~12 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| 5V | GPS VCC, SD module VCC, RTC VCC | |
| GND | GPS GND, SD module GND, RTC GND | shared ground |
| D4 | GPS TX -> Arduino RX (SoftwareSerial) | |
| D3 | GPS RX <- Arduino TX (SoftwareSerial) | GPS modules typically only need RX for optional config commands; read-only use is fine |
| D10 (SS) | SD module CS | |
| D11 | SD module MOSI | hardware SPI |
| D12 | SD module MISO | hardware SPI |
| D13 | SD module SCK | hardware SPI |
| A4 (SDA) | RTC SDA | |
| A5 (SCL) | RTC SCL | |
| D6 | Status LED via 220 Ω | blinks while searching, solid once GPS has a fix |

## How It Works

`TinyGPSPlus` parses the GPS module's raw NMEA sentence stream (a standardized text protocol GPS chips have used for decades) arriving over `SoftwareSerial`, exposing clean `.location.lat()`, `.location.lng()`, `.speed.kmph()`, and `.altitude.meters()` accessors, plus `.location.isValid()` to know whether the module has acquired a fix yet (this can take anywhere from a few seconds to a couple of minutes outdoors, and won't work reliably indoors at all).

The onboard RTC serves a specific, deliberate purpose here rather than being redundant with the GPS's own time data: GPS time is only valid once a fix is acquired, but the RTC keeps ticking through the entire "searching for satellites" period, so log rows recorded before a fix (marked with a `NO_FIX` placeholder for lat/lng) still get a sensible timestamp instead of a blank one. Once a fix is acquired, the sketch syncs the RTC to the GPS's UTC time so both clocks agree going forward.

A new row is appended to `TRACK.CSV` once per second while a fix is held, giving a fine-grained trail suitable for later import into mapping tools (most spreadsheet/mapping software can plot a lat/lng CSV directly).

## Setup & Flashing

1. Wire the GPS module, SD module, RTC, and status LED as above.
2. Install these libraries via Library Manager: `TinyGPSPlus` (by Mikal Hart) and `RTClib` (by Adafruit). `SD`, `SPI`, and `SoftwareSerial` are bundled with the IDE.
3. Format the microSD card as FAT32 before inserting it.
4. Open `src/gps_tracker.ino` in the Arduino IDE.
5. Select **Tools > Board > Arduino Uno** and the correct COM port, then upload.
6. Take the board outdoors with a clear sky view (GPS needs line-of-sight to satellites) and watch the status LED switch from blinking to solid once a fix is acquired; check the SD card afterward for `TRACK.CSV` rows.

## Extensions

- Add a small OLED or LCD showing live coordinates/speed instead of relying only on the SD log.
- Compute total distance traveled by summing the haversine distance between consecutive fixes.
- Combine with the dual-board wireless project's nRF24L01 pattern to transmit live position to a base station instead of only logging locally.
