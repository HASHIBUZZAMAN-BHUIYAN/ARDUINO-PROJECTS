# Wiring Notes — Obstacle-Avoiding Robot Car

```
Arduino Mega                 L298N Motor Driver
+------------+               +--------------------+
|        D22 |---------------| IN1 (left motor)   |
|        D23 |---------------| IN2 (left motor)   |
|         D5 |---------------| ENA (left PWM)      |
|        D24 |---------------| IN3 (right motor)  |
|        D25 |---------------| IN4 (right motor)  |
|         D6 |---------------| ENB (right PWM)     |
|        GND |---------------| GND                 |
+------------+               +--------------------+
                              12V IN  <- battery pack +
                              GND     <- battery pack -
                              OUT1/OUT2 -> left motor
                              OUT3/OUT4 -> right motor

Arduino Mega                 HC-SR04 (mounted on servo horn)
+------------+               +----------+
|         5V |---------------| VCC      |
|        GND |---------------| GND      |
|         D9 |---------------| TRIG     |
|        D10 |---------------| ECHO     |
+------------+               +----------+

Arduino Mega                 Pan Servo
+------------+               +----------+
|         5V |---------------| VCC      |
|        GND |---------------| GND      |
|        D11 |---------------| signal   |
+------------+               +----------+
```

- Tie the L298N's GND, the battery pack's GND, and the Mega's GND all together — without a common ground reference, the motor driver's logic signals won't be read correctly.
- If a motor spins the wrong direction, swap its two OUT wires at the L298N rather than rewiring the Arduino side.
- Mount the HC-SR04 on the servo horn so the servo can sweep it left/right to "look" both ways before turning.
