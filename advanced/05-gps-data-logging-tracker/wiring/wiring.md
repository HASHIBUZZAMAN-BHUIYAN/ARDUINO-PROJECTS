# Wiring Notes — GPS Data Logging Tracker

```
Arduino Uno                NEO-6M GPS
+-----------+              +----------+
|        5V |--------------| VCC      |
|       GND |----+---------| GND      |
|        D4 |--------------| TX       |   (GPS TX -> Arduino RX, via SoftwareSerial)
|        D3 |--------------| RX       |   (Arduino TX -> GPS RX, optional)
+-----------+    |         +----------+
                  |
Arduino Uno       |          microSD module (SPI)
+-----------+     |        +----------+
|        D10|--------------| CS       |
|        D11|--------------| MOSI     |
|        D12|--------------| MISO     |
|        D13|--------------| SCK      |
|        5V |--------------| VCC      |
|       GND |----+---------| GND      |
+-----------+    |         +----------+
                  |
Arduino Uno       |          DS3231 RTC (I2C)
+-----------+     |        +----------+
|     A4/SDA|--------------| SDA      |
|     A5/SCL|--------------| SCL      |
|        5V |--------------| VCC      |
|       GND |----+---------| GND      |
+-----------+                +----------+

     D6-[220ohm]--|>|-- Status LED --GND (blink=searching, solid=fix acquired)
```

- GPS modules need a clear view of the sky; testing indoors near a window may work, but a fix is far more reliable outdoors.
- The GPS module's RX line (Arduino D3 -> GPS RX) is optional for basic read-only tracking; it's included here in case you want to send configuration commands (e.g. changing the update rate) later.
- SD module CS is on D10 to match the Uno's standard hardware SPI SS pin; MOSI/MISO/SCK are fixed on 11/12/13 for any Uno-based hardware SPI device.
