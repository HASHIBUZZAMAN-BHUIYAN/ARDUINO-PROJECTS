# Wiring Notes — RS-485 Sensor Mesh with Uno Q Hub

```
                      Shared RS-485 bus (A, B, GND) - daisy chained
        +----------------+----------------+----------------+----------------+
        |                |                |                |
  +-----+-----+    +-----+-----+    +-----+-----+    +-----+-----+
  | MAX485 #1 |    | MAX485 #2 |    | MAX485 #3 |    | MAX485 #4 |
  | (Node 1)  |    | (Node 2)  |    | (Node 3)  |    | (Hub)     |
  +-----+-----+    +-----+-----+    +-----+-----+    +-----+-----+
        |                |                |                |
  Each MAX485: RO->D8(SoftSerial RX), DI->D9(SoftSerial TX), DE+RE->D2

Node 1 (Uno)         DHT22          BH1750 (I2C)
+--------+          +--------+     +----------+
|     D3 |----------| DATA   | A4--| SDA      |
|        |                    A5--| SCL      |
+--------+          +--------+     +----------+

Node 2 (Uno)         MQ-135          Sound sensor
+--------+          +--------+     +------------+
|     A0 |----------| AOUT   |     |            |
|     A1 |------------------------- -| AOUT      |
+--------+          +--------+     +------------+

Node 3 (Uno)         PIR (HC-SR501)   Reed switch
+--------+          +--------------+ +------------------+
|     D4 |----------| OUT          | | one leg -> D5    |
|     D5 |----------------------------- other leg -> GND (INPUT_PULLUP)
+--------+          +--------------+ +------------------+

Hub (Uno Q)          RTC (I2C)        SD (hw SPI)       W5500
+--------+          +----------+     +----------+      +--------+
| A4/A5  |----------| SDA/SCL  |     |          |      |        |
|     D4 |------------------------- --| CS       |      |        |
|     D7 |------------------------------------------- --| CS     |
+--------+          +----------+     +----------+      +--------+
```

- All 4 MAX485 modules share the same 2-wire A/B differential pair plus a common ground — never wire A/B backwards on any one node, or that node will fail to communicate while the others work fine.
- Terminate both physical ends of the bus with a 120Ω resistor across A/B if cable runs exceed a few meters.
- Each board's DE and RE pins are tied together to one digital pin so the sketch can flip between "transmit" (HIGH) and "listen" (LOW) with a single `digitalWrite`.
