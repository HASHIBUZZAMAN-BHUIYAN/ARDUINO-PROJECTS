# Wiring Notes — Smart Agriculture Command Platform

```
8x Soil Probes --> CD74HC4067 --(SIG)--> Mega A0
                       ^ (S0-S3 select: D22-D25)

Mega                 BME280 (I2C)    BH1750 (I2C)    RTC (I2C)
+--------+          +----------+    +----------+    +----------+
| 20/21  |----------| SDA/SCL  |----| SDA/SCL  |----| SDA/SCL  |
+--------+          +----------+    +----------+    +----------+

Mega                 8-ch relay (irrigation valves)
+--------+          +----------------------------------------+
| D30-D37|----------| IN1-IN8                                 |
+--------+          +----------------------------------------+

Mega                 4-ch relay (heater, fan, mister, grow-light)
+--------+          +----------------------------------------+
| D38-D41|----------| IN1-IN4                                 |
+--------+          +----------------------------------------+

Mega                 SD (hw SPI)         W5500 (shares SPI)
+--------+          +------------+      +------------+
|     53 |----------| CS         |      |            |
|     49 |----------------------------- --| CS         |
| 50/51/52 (shared MISO/MOSI/SCK) --------+------------+
+--------+
```

- This project reuses the multiplexed soil-zone wiring pattern from this tier's project 06, extended with a second relay bank for climate control.
- Keep the two relay banks' wiring physically separated from the multiplexer's select lines to avoid switching noise on the shared analog reading.
- Set `API_TOKEN` in the sketch to a real secret before any deployment beyond a local bench test.
