# Wiring Notes — RS-485 Distributed Greenhouse Zone Network

```
                      Shared RS-485 bus (A, B, GND) - daisy chained
        +----------------+----------------+----------------+
        |                |                |                |
  +-----+-----+    +-----+-----+    +-----+-----+    +-----+-----+
  | MAX485    |    | MAX485    |    | MAX485    |    | MAX485    |
  | (Zone 1)  |    | (Zone 2)  |    | (Zone 3)  |    | (Hub)     |
  +-----+-----+    +-----+-----+    +-----+-----+    +-----+-----+
        |                |                |                |
  Each MAX485: RO->D8(SoftSerial RX), DI->D9(SoftSerial TX), DE+RE->D2

Zone Controller (Uno)     DHT22        Soil probe      3-ch relay
+--------+               +--------+   +----------+    +----------------+
|     D3 |---------------| DATA   |   |          |    |                |
|     A0 |------------------------------ -| AOUT     |    |                |
|  D4-D6 |------------------------------------------- --| IN1-IN3       |
+--------+               +--------+   +----------+    +----------------+

Hub (Mega)                RTC (I2C)       SD (hw SPI)       W5500
+--------+                +----------+   +----------+      +--------+
| 20/21  |----------------| SDA/SCL  |
|     53 |------------------------------- --| CS       |
|     49 |----------------------------------------------- --| CS     |
+--------+                +----------+   +----------+      +--------+
```

- All 4 MAX485 modules share the same A/B pair and ground, same as the other RS-485 mesh project in this tier.
- Each zone's 3-channel relay drives its heater, fan, and mister independently based on that zone's own local PID output — no hub involvement in this path.
- Terminate both physical ends of the bus with 120Ω resistors for longer real-world runs.
