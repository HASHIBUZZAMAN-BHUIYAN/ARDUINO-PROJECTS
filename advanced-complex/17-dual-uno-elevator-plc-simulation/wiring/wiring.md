# Wiring Notes — Dual-Uno Elevator PLC Simulation

```
Car Controller (Uno, 0x20)      Limit switches (x3, INPUT_PULLUP)
+--------+                     +----------------------------+
| A4/A5  |------(I2C)--------->| (to Call Panel)              |
|     D2 |---------------------| Floor 1                     |
|     D3 |---------------------| Floor 2                     |
|     D4 |---------------------| Floor 3                     |
|     D5 |---------------------| Motor relay (UP)             |
|     D6 |---------------------| Motor relay (DOWN)           |
|     D7 |---------------------| Door-open indicator           |
+--------+                     +----------------------------+

Call Panel (Uno)                Call buttons (x3)   7-seg display   Direction LEDs
+--------+                     +----------------+   +------------+ +--------------+
| A4/A5  |------(I2C)--------->| (to Car Ctrl)  |
|     D2 |---------------------| Floor 1 call   |
|     D3 |---------------------| Floor 2 call   |
|     D4 |---------------------| Floor 3 call   |
|  D5-D7 |------------------------------------- --| segments   |
|     D8 |---------------------------------------------------- -| UP LED        |
|     D9 |---------------------------------------------------- -| DOWN LED      |
+--------+                     +----------------+   +------------+ +--------------+
```

- Both boards' I2C pins (A4/A5) are wired together directly with a shared ground and one pull-up resistor pair on the bus.
- Limit switches are wired `INPUT_PULLUP` so they read HIGH when open and LOW when the car is physically at that floor.
- A simple demo build can substitute the "motor + limit switches" with a servo sweeping between 3 marked positions and micro-switches at each, if a full linear rail isn't available.
