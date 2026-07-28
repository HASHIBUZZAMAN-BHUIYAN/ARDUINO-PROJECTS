# Wiring Notes — PID Greenhouse Climate Controller

```
Mega                       DHT22                BH1750 (I2C)         Soil Probe
+---------+              +--------+            +----------+        +----------+
|      5V |--------------| VCC    |    20(SDA)--| SDA      |   5V --| VCC      |
|     GND |----+---------| GND    |    21(SCL)--| SCL      |  GND --| GND      |
|      D2 |--------------| DATA   |     GND ----| GND      |   A0 --| AOUT     |
+---------+              +--------+     5V  ----| VCC      |        +----------+
                                         +----------+

Mega                       4-Channel Relay Module
+---------+              +------------------------+
|      D3 |--------------| IN1 (Heater)           |
|      D4 |--------------| IN2 (Fan)               |
|      D5 |--------------| IN3 (Mister)            |
|      D6 |--------------| IN4 (Grow-light)         |
|      5V |--------------| VCC                     |
|     GND |--------------| GND                     |
+---------+              +------------------------+

Mega                       SD Module          W5500 Ethernet
+---------+              +----------+        +---------------+
|   50/51/52 (shared SPI bus) -------|--------|  MISO/MOSI/SCK |
|      53 |----------------| CS      |        |                |
|      49 |----------------------------------| CS              |
+---------+              +----------+        +---------------+
```

- BH1750 and DS3231 both sit on the Mega's dedicated I2C pins (20/21), not A4/A5 as on an Uno.
- SD card and Ethernet share the hardware SPI bus (pins 50/51/52) but must use two distinct CS pins (53 for SD, 49 for Ethernet) — never assert both at once, which the Ethernet library and SD library both handle correctly as long as CS pins are distinct and each library's `begin()` is called with its own CS pin.
- Relay module inputs are active-LOW on most cheap boards — verify polarity before assuming `HIGH` energizes the relay; the sketch's `RELAY_ACTIVE_LOW` constant handles this in software.
