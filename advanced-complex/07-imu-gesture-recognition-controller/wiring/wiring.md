# Wiring Notes — IMU Gesture Recognition Controller

```
Nano             MPU6050 (I2C)
+--------+      +-----------+
|     A4 |------| SDA       |
|     A5 |------| SCL       |
|     5V |------| VCC       |
|    GND |------| GND       |
+--------+      +-----------+

Nano             Pan servo     Tilt servo    Relay (light)   Cal button
+--------+      +---------+   +---------+   +------------+ +------------------+
|     D9 |------| signal  |   |         |   |            | |                  |
|    D10 |----------------- --| signal  |   |            | |                  |
|     D6 |--------------------------------- -| IN         | |                  |
|     D2 |------------------------------------------------- -| (INPUT_PULLUP) |
+--------+      +---------+   +---------+   +------------+ +------------------+
```

- Power both servos from the Nano's 5V pin only if they're small micro servos with modest stall current; for anything larger, use an external 5V supply with shared ground.
- Mount the MPU6050 rigidly to whatever the gesture controller is embedded in (handheld remote, wearable strap, etc.) — a loose sensor produces inconsistent features.
- The calibration button only needs one leg wired (the other to GND) since it uses `INPUT_PULLUP`.
