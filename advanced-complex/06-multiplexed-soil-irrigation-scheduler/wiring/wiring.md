# Wiring Notes — Multiplexed 8-Zone Irrigation Scheduler

```
8x Soil Probes --> CD74HC4067 channels C0-C7

Mega                CD74HC4067
+--------+         +------------------+
|     A0 |---------| SIG              |
|    D22 |---------| S0               |
|    D23 |---------| S1               |
|    D24 |---------| S2               |
|    D25 |---------| S3               |
|     5V |---------| VCC              |
|    GND |---------| GND, EN (tied low)|
+--------+         +------------------+

Mega                Rain sensor        RTC (I2C)
+--------+         +------------+     +----------+
|     D2 |---------| DO         |     |          |
| 20/21  |------------------------- --| SDA/SCL  |
+--------+         +------------+     +----------+

Mega                8-ch relay module -> 8x solenoid valves
+--------+         +----------------------------------------+
| D30-D37|---------| IN1-IN8                                 |
+--------+         +----------------------------------------+

Mega                SD (hw SPI)        W5500 (shares SPI)
+--------+         +------------+     +------------+
|     53 |---------| CS         |     |            |
|     49 |------------------------- --| CS          |
| 50/51/52 (shared MISO/MOSI/SCK) ------------------+
+--------+
```

- The CD74HC4067's `EN` pin is active-LOW; tie it to GND to keep the multiplexer permanently enabled.
- Solenoid valves typically need 12/24V and draw more current than the relay module's logic side — confirm the relay module's contact rating matches your valves and power them from a separate supply, sharing ground with the Mega.
- Keep multiplexer select-line wiring short and away from the valve relay wiring to avoid switching noise affecting the analog reading.
