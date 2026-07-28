# DIY Weather Station

Combines a temperature/humidity sensor and a barometric pressure sensor, displaying live readings plus a simple rising/falling pressure trend on a 16x2 LCD.

## Difficulty & Board

**Tier:** Intermediate
**Board:** Arduino Uno

Reasoning: two I2C/digital sensors plus a parallel LCD is a moderate pin count that the Uno handles comfortably; no motors or high pin-count peripherals are involved so there's no need to reach for the Mega.

## Components

| Part | Qty |
|---|---|
| Arduino Uno | 1 |
| DHT22 temperature/humidity sensor | 1 |
| BMP280 barometric pressure sensor (I2C) | 1 |
| 16x2 character LCD (HD44780, parallel) | 1 |
| 10 kΩ potentiometer (LCD contrast) | 1 |
| 10 kΩ resistor (DHT22 pull-up, if not on breakout) | 1 |
| Breadboard | 1 |
| Jumper wires | ~16 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| 5V | DHT22 VCC, LCD VDD | |
| GND | DHT22 GND, LCD VSS, BMP280 GND | shared ground |
| D2 | DHT22 DATA | with 10kΩ pull-up to 5V if using a bare sensor (breakout boards usually include it) |
| A4 (SDA) | BMP280 SDA | Uno's I2C data line |
| A5 (SCL) | BMP280 SCL | Uno's I2C clock line |
| 3V3 | BMP280 VCC | most breakout modules are 3.3V-only, check yours |
| D7 | LCD RS | |
| D8 | LCD EN | |
| D9 | LCD D4 | |
| D10 | LCD D5 | |
| D11 | LCD D6 | |
| D12 | LCD D7 | |
| Pot wiper | LCD V0 (contrast) | pot outer legs to 5V and GND |

## How It Works

The sketch reads the DHT22 (temperature + humidity) and BMP280 (pressure + its own onboard temperature) every 2 seconds — DHT22 sensors can't be polled faster than ~1Hz reliably, which sets the overall refresh rate. Readings are written to the LCD across two lines: temperature/humidity on line 1, pressure on line 2.

To show a **trend**, the sketch keeps a small rolling history of the last few pressure readings in an array. Each cycle it compares the current reading to the oldest one in that history: if pressure has risen more than a small threshold, it shows an "up" arrow character; if it's dropped, a "down" arrow; otherwise "steady". This is a classic technique for turning noisy point-in-time sensor data into a more useful "is it changing" signal without needing to store long-term data (a distinguishing feature vs. the basic-tier single-sensor projects, which only ever look at the instantaneous reading).

## Setup & Flashing

1. Wire the DHT22, BMP280, and LCD as above; adjust the contrast pot until text is visible.
2. Install libraries via **Tools > Manage Libraries**: `DHT sensor library` (by Adafruit) and its dependency `Adafruit Unified Sensor`, plus `Adafruit BMP280 Library` and its dependency `Adafruit BusIO`. See `libraries.txt`.
3. Open `src/weather_station.ino` in the Arduino IDE.
4. Select **Tools > Board > Arduino Uno** and the correct COM port.
5. Upload and confirm the LCD shows live temperature, humidity, and pressure with a trend arrow that updates over a few minutes.

## Extensions

- Add an SD card + RTC to log readings over days/weeks (see the advanced-tier Mega dashboard for a fuller data-logging design).
- Add a "comfort index" calculation (heat index) that combines temperature and humidity.
- Push readings out over serial to a laptop and graph them.
