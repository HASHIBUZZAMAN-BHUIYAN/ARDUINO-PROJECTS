# Wiring Notes — I2C Multi-Drop Industrial Node Network

```
                Shared I2C bus: SDA -- SDA -- SDA -- SDA
                                SCL -- SCL -- SCL -- SCL
                                GND -- GND -- GND -- GND
                 (one 4.7k pull-up pair on SDA and SCL, anywhere on the bus)

Nano (Relay Bank, 0x08)      4-ch relay module
+--------+                  +----------------+
| A4/A5  |------------------|                |
|  D4-D7 |------------------| IN1-IN4        |
+--------+                  +----------------+

Nano (Load Cell, 0x09)       HX711            Load cell
+--------+                  +--------+        +----------+
| A4/A5  |------------------|        |        |          |
|     D2 |------------------| DOUT   |--------| (4-wire) |
|     D3 |------------------| SCK    |
+--------+                  +--------+        +----------+

Nano (Thermocouple, 0x0A)    MAX6675          K-type thermocouple
+--------+                  +--------+        +------------------+
| A4/A5  |------------------|        |        |                  |
|     D8 |------------------| CS     |--------| (2-wire)         |
|    D11 |------------------| SO     |
|    D13 |------------------| SCK    |
+--------+                  +--------+        +------------------+

Uno Q (Hub)                 RTC (I2C)         SD (SPI)        W5500
+--------+                  +----------+      +--------+      +--------+
| A4/A5  |------------------| SDA/SCL  |      |        |      |        |
|     D9 |------------------------------------| CS     |      |        |
|     D7 |------------------------------------------------- --| CS     |
+--------+                  +----------+      +--------+      +--------+
```

- Each Nano sets its I2C slave address in `Wire.begin(addr)`; addresses are chosen in the unreserved 0x08-0x77 range.
- Only one pull-up resistor pair is needed on the whole bus (not one per node) — adding pull-ups at every node over-loads the bus.
- Keep total bus wiring short (well under a meter total) for reliable multi-drop I2C; see the README's Known Limitations for the RS-485 alternative at larger scale.
