# Mega Multi-Sensor Environmental Dashboard

A single Mega ties together five sensors (temperature/humidity, barometric pressure, gas/air-quality, light, and sound level), shows them on a 20x4 LCD, and logs a timestamped row to an SD card once a minute using an RTC for accurate timekeeping.

## Difficulty & Board

**Tier:** Advanced
**Board:** Arduino Mega 2560

Reasoning: this is the flagship "everything at once" pin-count project — DHT22, BMP280, MQ-2 gas sensor, LDR, sound sensor, a 20x4 LCD (6 control pins), a DS3231 RTC, and an SD card module (SPI, 4 pins) adds up to well over a dozen I/O lines. An Uno's ~20 usable pins would be uncomfortably tight once you add power/ground runs; the Mega's 54 digital + 16 analog pins give comfortable headroom, exactly the "pin-heavy multi-sensor" case the Mega is favored for in this repo.

## Components

| Part | Qty |
|---|---|
| Arduino Mega 2560 | 1 |
| DHT22 temperature/humidity sensor | 1 |
| BMP280 barometric pressure sensor (I2C) | 1 |
| MQ-2 gas/smoke sensor (analog) | 1 |
| LDR light sensor + 10 kΩ resistor | 1 |
| Sound level sensor module (analog out) | 1 |
| DS3231 RTC module (I2C) | 1 |
| microSD card module (SPI) + microSD card | 1 |
| 20x4 character LCD (HD44780, parallel) | 1 |
| 10 kΩ potentiometer (LCD contrast) | 1 |
| Breadboard / protoboard | 1 |
| Jumper wires | ~30 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| D2 | DHT22 DATA | |
| 3V3 | BMP280 VCC | most breakouts are 3.3V-only |
| 20 (SDA) | BMP280 SDA, DS3231 SDA | Mega's dedicated I2C pins (not A4/A5 like the Uno) |
| 21 (SCL) | BMP280 SCL, DS3231 SCL | |
| A0 | MQ-2 AOUT | |
| A1 | LDR voltage divider midpoint | with 10kΩ to GND |
| A2 | Sound sensor AOUT | |
| 53 (SS) | SD module CS | Mega's default hardware SS pin |
| 51 | SD module MOSI | Mega's hardware SPI |
| 50 | SD module MISO | Mega's hardware SPI |
| 52 | SD module SCK | Mega's hardware SPI |
| D22-D27 | 20x4 LCD RS, EN, D4-D7 | 6 control lines |

## How It Works

Every loop, the sketch reads all five sensors and pulls the current time from the DS3231 RTC (a battery-backed real-time clock chip, so it keeps accurate time even when the Mega itself is powered off). The 20x4 LCD cycles through two screens automatically every 4 seconds (screen 1: temperature/humidity/pressure, screen 2: gas/light/sound), since a 20x4 display can't fit all five readings clearly at once.

Once a minute (tracked using the RTC's timestamp rather than `millis()`, so logging intervals stay accurate even across resets), the sketch appends one CSV row — timestamp, temperature, humidity, pressure, gas level, light level, sound level — to a file on the SD card, creating the file with a header row the first time it doesn't yet exist. This combination of **multi-sensor fusion + persistent timestamped logging** is what distinguishes this from every earlier single- or dual-sensor project in the repo, and mirrors how a real environmental monitoring station is built.

## Setup & Flashing

1. Wire all five sensors, the RTC, the SD module, and the LCD as above. Double-check the BMP280 and DS3231 share the Mega's I2C bus on pins 20/21 (distinct from analog pins A4/A5 used for I2C on an Uno).
2. Install these libraries via Library Manager: `DHT sensor library` + `Adafruit Unified Sensor`, `Adafruit BMP280 Library` + `Adafruit BusIO`, `RTClib` (by Adafruit), and `SD` (bundled with the IDE). See `libraries.txt`.
3. Format the microSD card as FAT32 before inserting it.
4. Open `src/multisensor_dashboard.ino` in the Arduino IDE.
5. Select **Tools > Board > Arduino Mega or Mega 2560** and the correct COM port.
6. The first time you upload, uncomment the `rtc.adjust(...)` line noted in the code to set the RTC to your computer's current time, upload once, then comment it back out and re-upload — otherwise the RTC resets to compile-time every power cycle.
7. Confirm the LCD cycles both screens and that a `LOG.CSV` file appears on the SD card with new rows appended roughly once a minute.

## Extensions

- Add a WiFi/Ethernet uplink (see the advanced-tier Ethernet or ESP8266 projects in this repo) to push readings to a cloud dashboard instead of only logging locally.
- Add threshold-based alerts (buzzer/LED) for dangerous gas levels, reusing the basic-tier alarm pattern.
- Graph the logged CSV data afterward in a spreadsheet to visualize daily environmental cycles.
