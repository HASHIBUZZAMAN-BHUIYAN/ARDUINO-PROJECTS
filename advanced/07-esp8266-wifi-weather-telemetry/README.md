# ESP8266 WiFi Weather Telemetry

Reads temperature/humidity/pressure and pushes each reading over WiFi as an HTTP GET request to a local server (e.g. a simple logging endpoint on a home server/Raspberry Pi), using a low-cost ESP8266 (ESP-01) module as a WiFi co-processor.

## Difficulty & Board

**Tier:** Advanced
**Board:** Arduino Uno Q

Reasoning: the ESP8266 co-processor pattern (main MCU talks AT commands over a serial UART to a separate WiFi chip) is the most broadly portable, well-documented way to add WiFi to an Arduino-family sketch, and it stacks cleanly onto the Uno Q's Uno-form-factor headers. Keeping WiFi on a discrete, swappable module (rather than depending on board-specific onboard networking APIs) keeps this sketch reusable across boards. This is the repo's WiFi-based telemetry project, distinct from the wired-Ethernet home automation server elsewhere in this tier.

## Components

| Part | Qty |
|---|---|
| Arduino Uno Q | 1 |
| ESP8266 ESP-01 WiFi module | 1 |
| 3.3V regulator module (dedicated ESP-01 power) | 1 |
| DHT22 temperature/humidity sensor | 1 |
| BMP280 barometric pressure sensor (I2C) | 1 |
| Logic-level-safe voltage divider parts (2x resistors) | 2 |
| Breadboard | 1 |
| Jumper wires | ~12 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| 3V3 | ESP-01 CH_PD (via dedicated 3.3V regulator, see notes) | ESP-01 draws current spikes an Arduino's onboard 3.3V pin often can't supply reliably |
| D2 | DHT22 DATA | |
| 3V3 | BMP280 VCC | |
| A4 (SDA) | BMP280 SDA | |
| A5 (SCL) | BMP280 SCL | |
| D8 | ESP-01 RXD | via a 2-resistor divider stepping the Uno Q's TX down if needed (see wiring notes; Uno Q is already 3.3V logic so a divider is often unnecessary — verify with your specific board revision) |
| D9 | ESP-01 TXD | direct to Arduino RX, both sides 3.3V |

## How It Works

The `WiFiEspAT` library presents the same familiar `WiFi.h`-style API (`WiFi.begin()`, `WiFiClient`, etc.) that Arduino WiFi-shield sketches use, but underneath it talks to the ESP8266 over a plain serial UART using **AT commands** — short text commands like `AT+CWJAP` (join access point) that the ESP8266's stock firmware understands. This lets the sketch stay almost identical to "native WiFi" example code while using a cheap, widely available co-processor module instead of requiring board-specific WiFi hardware.

Every 60 seconds the sketch:

1. Reads temperature, humidity, and pressure from the DHT22/BMP280 (same sensor combination as the intermediate weather station).
2. Builds a simple HTTP GET request with the readings encoded as URL query parameters (e.g. `/log?t=21.4&h=48&p=1013`).
3. Opens a `WiFiClient` connection to the configured server/port, sends the request, and reads back the response status line for a basic success/failure check logged to Serial.
4. Closes the connection (short-lived requests like this don't need to hold the socket open between readings).

This project's core new concept is a wireless network transport for sensor data — the same DHT22+BMP280 reading logic from the intermediate weather station, now leaving the device entirely instead of only being shown on a local LCD.

## Setup & Flashing

1. Wire the ESP-01 (through its own dedicated 3.3V regulator — do not power it from the Arduino's onboard 3.3V pin, which usually can't supply the ESP8266's current spikes reliably), DHT22, and BMP280 as above.
2. Install these libraries via Library Manager: `WiFiEspAT` (by Juraj Andrássy), `DHT sensor library` + `Adafruit Unified Sensor`, `Adafruit BMP280 Library` + `Adafruit BusIO`.
3. Open `src/wifi_weather_telemetry.ino`, and fill in `WIFI_SSID`, `WIFI_PASSWORD`, `SERVER_HOST`, and `SERVER_PORT` for your own network and logging endpoint (a simple local script that just appends received query parameters to a file is enough to test with — no real credentials are included in this sketch).
4. Select **Tools > Board > Arduino Uno Q** and the correct COM port, then upload.
5. Open the Serial Monitor at 9600 baud and confirm the sketch reports a successful WiFi connection and successful HTTP responses each cycle.

## Extensions

- Switch the HTTP GET pattern to MQTT publish (via a library like `PubSubClient`) for a more efficient, purpose-built IoT telemetry protocol.
- Add local buffering (small in-memory queue) so readings aren't lost during a brief WiFi outage, sending them once connectivity returns.
- Combine with the advanced-tier Mega dashboard's SD logging so readings are captured both locally and remotely.
