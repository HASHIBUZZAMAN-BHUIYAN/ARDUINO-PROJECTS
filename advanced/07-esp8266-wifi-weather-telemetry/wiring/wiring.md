# Wiring Notes — ESP8266 WiFi Weather Telemetry

```
Dedicated 3.3V regulator        ESP-01 (ESP8266)
+-----------+                  +----------+
|      OUT+ |------------------| VCC      |
|      OUT+ |------------------| CH_PD    |  (tie CH_PD high to enable the module)
|      OUT- |------------------| GND      |
+-----------+                  +----------+

Arduino Uno Q                   ESP-01
+-----------+                  +----------+
|        D8 |------------------| RXD      |   (Arduino TX -> ESP RX)
|        D9 |------------------| TXD      |   (ESP TX -> Arduino RX)
+-----------+                  +----------+
   Regulator GND, ESP GND, and Arduino GND must all be tied together.

Arduino Uno Q                   DHT22                BMP280 (I2C)
+-----------+                  +----------+          +----------+
|        D2 |------------------| DATA     |          |          |
|       3V3 |------------------| VCC      |----------| VCC      |
|       GND |----+-------------| GND      |----+-----| GND      |
|     A4/SDA|------------------------------------------ SDA      |
|     A5/SCL|------------------------------------------ SCL      |
+-----------+                  +----------+          +----------+
```

- The ESP-01 can draw current spikes (up to ~300mA) during WiFi transmission that most Arduino boards' onboard 3.3V regulators cannot reliably supply — use a separate dedicated 3.3V regulator module for the ESP-01, with its ground tied back to the Arduino's ground.
- The Uno Q's own GPIO logic level is already 3.3V, matching the ESP-01's UART directly — a resistor divider on the TX line (needed when pairing an ESP-01 with a 5V-logic board like a classic Uno) is not required here, but is mentioned in the README for portability to other boards.
- If AT commands seem to fail intermittently, double-check the ESP-01's baud rate matches what the sketch expects (many modules ship at 115200 baud by default; consider reflashing to 9600 baud firmware, or configure `WiFiEspAT`'s serial port to match 115200 instead).
