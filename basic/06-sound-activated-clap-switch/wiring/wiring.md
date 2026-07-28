# Wiring Notes — Sound-Activated Clap Switch

```
Arduino Nano             Sound Sensor (KY-038 etc.)
+-----------+           +--------------------------+
|        5V |-----------| VCC                      |
|       GND |----+------| GND                      |
|         D2|-----------| DOUT                     |
+-----------+    |      +--------------------------+
                 |
                 |        1-Channel Relay Module
       5V --------------- VCC
       D4 --------------- IN
      GND --------------- GND
                          COM/NO -> LED (+resistor) -> GND (simulated "lamp")
```

- Set the sound sensor's onboard potentiometer so a normal clap from ~1m away reliably fires `DOUT`, but background noise doesn't.
- For this beginner build, drive only a low-voltage LED through the relay's NO/COM contacts — do not wire mains-voltage appliances through it unless you are experienced with mains safety.
- Most low-cost relay modules are active-LOW on `IN` (LOW = energized) — verify with your specific module and adjust the sketch's `RELAY_ON`/`RELAY_OFF` constants if needed.
