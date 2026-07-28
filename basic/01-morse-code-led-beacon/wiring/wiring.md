# Wiring Notes — Morse Code LED Beacon

```
Arduino Uno                     LED
+-----------+                 anode (long leg)
|        D8 |----[220 ohm]-----|>|-----+
|           |                 cathode  |
|       GND |-------------------------+
+-----------+
```

- The 220 ohm resistor can sit on either leg of the LED; placing it on the anode side (as drawn) is convention only.
- Double-check LED polarity: the longer leg (anode) goes toward the resistor/D8 side, the shorter leg (cathode) goes to GND.
- No external power supply needed — USB power from the host computer is sufficient.
