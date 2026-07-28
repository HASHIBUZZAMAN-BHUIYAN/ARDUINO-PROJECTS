# Wiring Notes — DIY Weather Station

```
Arduino Uno                DHT22
+-----------+              +----------+
|        5V |--------------| VCC      |
|      GND  |----+---------| GND      |
|        D2 |--[10k pullup to 5V]-----| DATA |
+-----------+    |         +----------+
                  |
Arduino Uno       |          BMP280 (I2C)
+-----------+     |         +----------+
|       3V3 |---------------| VCC      |
|       GND |----+----------| GND      |
|     A4/SDA|---------------| SDA      |
|     A5/SCL|---------------| SCL      |
+-----------+

Arduino Uno                16x2 LCD (HD44780)
+-----------+              +----------+
|        5V |--------------| VDD      |
|       GND |--------------| VSS      |
|        D7 |--------------| RS       |
|        D8 |--------------| EN       |
|        D9 |--------------| D4       |
|       D10 |--------------| D5       |
|       D11 |--------------| D6       |
|       D12 |--------------| D7       |
+-----------+              +----------+
    Contrast pot wiper -----> LCD V0
    Pot outer legs -----> 5V and GND
    LCD R/W -> GND (write-only mode)
    LCD A/K (backlight) -> 5V / GND via appropriate resistor if not built into your module
```

- Double-check your BMP280 breakout's voltage rating — most common modules are 3.3V logic; powering from 5V can damage them. Some breakouts include an onboard regulator/level shifter and are 5V-tolerant — check the silkscreen/datasheet.
- DHT22 needs a 10k pull-up resistor between DATA and VCC if your breakout doesn't already include one (most 3-pin DHT22 modules do include it).
