# Wiring Notes — IR Remote Control Hub

```
Arduino Uno Q             IR Receiver (TSOP38238)
+-----------+             +----------+
|       3V3 |-------------| VCC      |
|       GND |----+--------| GND      |
|        D2 |-------------| OUT      |
+-----------+    |        +----------+
                  |
Arduino Uno Q     |         4-Channel Relay Module
+-----------+     |        +----------+
|        D4 |--------------| IN1      |
|        D5 |--------------| IN2      |
|        D6 |--------------| IN3      |
|        D7 |--------------| IN4      |
|        5V |--------------| VCC      |
|       GND |----+---------| GND      |
+-----------+              +----------+
   Each relay COM/NO -> an LED (+resistor) -> GND, standing in for an appliance
```

- D2 is used for the IR receiver because it's one of the Uno Q's interrupt-capable pins, which the `IRremote` library relies on for accurate pulse timing.
- All logic here is 3.3V; confirm your specific relay module's `IN` pins reliably trigger from a 3.3V HIGH (most opto-isolated modules do; some non-isolated ones expect a full 5V and may need a small transistor buffer).
