# Wiring Notes — Autonomous Water Quality Monitoring Buoy

```
Uno Q            pH sensor      Turbidity sensor   DO sensor       DS18B20 (OneWire)
+--------+      +----------+   +----------------+ +-----------+  +------------------+
|     A0 |------| AOUT     |
|     A1 |------------------- --| AOUT           |
|     A2 |----------------------------------------- -| AOUT      |
|     D2 |----------------------------------------------------------- -| DATA (4.7k pull-up)|
+--------+      +----------+   +----------------+ +-----------+  +------------------+

Uno Q            HC-SR04 (water level)     INA219 (I2C)      RTC (I2C)
+--------+      +----------------+        +----------+      +----------+
|     D6 |------| TRIG           |   A4/A5-| SDA/SCL  |------| SDA/SCL  |
|     D7 |------| ECHO           |
+--------+      +----------------+        +----------+      +----------+

Uno Q            Aerator relay      Shore alarm relay      SD (hw SPI)     W5500
+--------+      +--------------+   +------------------+   +------------+ +--------+
|     D8 |------| IN           |
|     D9 |---------------------- --| IN                |
|    D10 |------------------------------------------------- -| CS         |
| D11-13 |------------------------------------------------------------------| MISO/MOSI/SCK|
|     D5 |----------------------------------------------------------------------------- --| CS     |
+--------+      +--------------+   +------------------+   +------------+ +--------+
```

- Mount pH/turbidity/DO sensor tips fully submerged at a consistent depth; the DS18B20 should be near the other probes for a representative local water temperature.
- All submerged electronics/connectors need waterproof potting or enclosures rated for continuous immersion — a buoy deployment is far less forgiving of a loose connection than a bench build.
- The W5500 shield assumes a shore-side bridge (e.g. a point-to-point WiFi-to-Ethernet bridge on the dock) providing the actual network path back to shore; see Known Limitations for the open-water/cellular alternative.
