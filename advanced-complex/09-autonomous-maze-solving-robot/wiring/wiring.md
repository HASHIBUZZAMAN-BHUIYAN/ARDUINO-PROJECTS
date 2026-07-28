# Wiring Notes — Autonomous Maze-Solving Robot

```
Mega             IR sensors (x3)        Ultrasonic (HC-SR04)
+--------+      +----------------+     +----------------+
|  A0-A2 |------| AOUT x3        |  D6-| TRIG            |
|        |      +----------------+  D7-| ECHO            |
+--------+                          +----------------+

Mega             Left encoder       Right encoder      MPU6050 (I2C)
+--------+      +------------+     +------------+     +-----------+
|     D2 |------| A (INT)    | D18-| A (INT)     | 20--| SDA       |
|     D3 |------| B          | D19-| B           | 21--| SCL       |
+--------+      +------------+     +------------+     +-----------+

Mega             L298N motor driver
+--------+      +----------------------------+
|     D8 |------| IN1 (left fwd)              |
|     D9 |------| IN2 (left rev)               |
|    D10 |------| IN3 (right fwd)              |
|    D11 |------| IN4 (right rev)              |
+--------+      +----------------------------+
```

- Encoder A channels use hardware interrupt pins (D2, D18) for reliable tick-counting; B channels are read at interrupt time to determine direction.
- Mount IR sensors angled slightly outward on the front-left/front-right to detect walls just before entering a cell, and the side sensor perpendicular for side-wall detection.
- The MPU6050's yaw drifts slowly over time even when stationary — the sketch periodically re-zeroes yaw drift using the encoder-implied heading when driving straight, documented in code comments.
