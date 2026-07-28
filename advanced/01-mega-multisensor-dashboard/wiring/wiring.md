# Wiring Notes — Mega Multi-Sensor Environmental Dashboard

```
Arduino Mega              DHT22             MQ-2            LDR (+10k to GND)      Sound sensor
+-----------+            +--------+       +--------+       +----------------+     +--------+
|        5V |------------| VCC    |       | VCC    |       | 5V leg         |     | VCC    |
|       GND |----+-------| GND    |---+---| GND    |---+---| GND (via 10k)  |--+--| GND    |
|         D2|------------| DATA   |   |   |        |   |   |                |  |  |        |
|         A0|--------------------------- AOUT       |   |                |  |  |        |
|         A1|------------------------------------------------ divider mid |  |  |        |
|         A2|------------------------------------------------------------------| AOUT   |
+-----------+                                                                  +--------+

Arduino Mega              BMP280 (I2C)      DS3231 RTC (I2C)
+-----------+            +----------+      +----------+
|       3V3 |------------| VCC      |------| VCC      |
|       GND |------------| GND      |------| GND      |
|      20   |------------| SDA      |------| SDA      |   (Mega dedicated I2C pins)
|      21   |------------| SCL      |------| SCL      |
+-----------+            +----------+      +----------+

Arduino Mega              microSD module (SPI)
+-----------+            +----------+
|         53|------------| CS       |
|         51|------------| MOSI     |
|         50|------------| MISO     |
|         52|------------| SCK      |
|        5V |------------| VCC      |
|       GND |------------| GND      |
+-----------+            +----------+

Arduino Mega              20x4 LCD
+-----------+            +----------+
|        D22|------------| RS       |
|        D23|------------| EN       |
|        D24|------------| D4       |
|        D25|------------| D5       |
|        D26|------------| D6       |
|        D27|------------| D7       |
+-----------+            +----------+
   Contrast pot wiper -> LCD V0; pot outer legs -> 5V/GND; LCD R/W -> GND
```

- The Mega's hardware I2C bus is on pins 20 (SDA) and 21 (SCL), not A4/A5 as on an Uno — both the BMP280 and DS3231 share this same bus since I2C supports multiple devices on one pair of lines, each with its own address.
- The Mega's hardware SPI bus (used by the SD module) is fixed on pins 50-53, unlike the Uno's 11-13.
- Give the MQ-2 gas sensor a warm-up period of a few minutes after first power-up; its readings drift while the sensing element heats up to operating temperature.
