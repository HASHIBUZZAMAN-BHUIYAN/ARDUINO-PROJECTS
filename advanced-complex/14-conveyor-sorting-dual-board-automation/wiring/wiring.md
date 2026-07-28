# Wiring Notes — Conveyor Sorting Dual-Board Automation

```
Uno (Sorter, 0x10)      TCS3200               Diverter servo
+--------+             +----------------+    +------------+
| A4/A5  |------------->| (I2C to Mega)  |    |            |
| D2-D5  |-------------| S0-S3          |    |            |
|     D6 |-------------| OUT            |    |            |
|     D9 |-------------------------------- --| signal      |
+--------+             +----------------+    +------------+

Mega (Line Controller)   L298N motor driver     IR break-beam
+--------+               +----------------+    +----------------+
| 20/21  |-------------->| (I2C to Uno)   |    |                |
|     D8 |---------------| IN1            |    |                |
|     D9 |---------------| IN2            |    |                |
|    D10 |---------------| ENA (PWM)      |    |                |
|     D2 |------------------------------------- -| OUT (INT)     |
+--------+               +----------------+    +----------------+
```

- The I2C bus between the Uno (A4/A5) and Mega (20/21, its dedicated I2C pins, not A4/A5) needs one shared pull-up pair and a common ground.
- Mount the IR break-beam pair so items (or a fixed reflective marker on the belt) interrupt it once per revolution/spacing interval for consistent speed feedback.
- Power the conveyor DC motor from the L298N's motor supply rail, not from either Arduino's 5V pin.
