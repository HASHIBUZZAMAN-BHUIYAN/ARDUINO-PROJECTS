# Wiring Notes — Multi-Sensor Weather Buoy Datalogger

```
Uno Q            Anemometer        Rain gauge        Wind vane      UV sensor
+--------+      +------------+    +------------+    +----------+   +----------+
|     D2 |------| reed pulse |    |            |    |          |   |          |
|     D3 |---------------------- -| reed pulse |    |          |   |          |
|     A0 |------------------------------------------ -| wiper   |   |          |
|     A1 |------------------------------------------------------ --| AOUT     |
+--------+      +------------+    +------------+    +----------+   +----------+

Uno Q            HC-SR04 (water level)      BME280 (I2C)    RTC (I2C)
+--------+      +----------------+         +----------+    +----------+
|     D6 |------| TRIG           |    A4/A5-| SDA/SCL  |----| SDA/SCL |
|     D7 |------| ECHO           |
+--------+      +----------------+

Uno Q            SD (hw SPI)              W5500 (shares SPI)
+--------+      +----------------+       +--------------+
|    D10 |------| CS             |       |              |
|    D11 |------| MOSI           |       |              |
|    D12 |------| MISO           |       |              |
|    D13 |------| SCK            |       |              |
|     D9 |------------------------- -----| CS           |
+--------+      +----------------+       +--------------+
```

- Both the anemometer and rain gauge are simple magnetic reed switches that pulse to ground on each rotation/tip — wire them with `INPUT_PULLUP` so an open switch reads HIGH and a closed switch pulls the pin LOW.
- Mount the HC-SR04 facing straight down over the water surface for level sensing; keep it clear of spray/splash that could produce false echoes.
- Use outdoor-rated, shielded cable runs for the anemometer/vane/rain-gauge if the buoy enclosure is more than a couple of meters from the mounting mast.
