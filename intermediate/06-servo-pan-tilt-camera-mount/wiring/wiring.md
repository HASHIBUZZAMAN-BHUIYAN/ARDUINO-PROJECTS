# Wiring Notes — Servo Pan-Tilt Camera Mount

```
Arduino Nano              2-Axis Joystick
+-----------+             +----------+
|        5V |-------------| VCC      |
|       GND |-------------| GND      |
|        A0 |-------------| VRx      |
|        A1 |-------------| VRy      |
|        D8 |-------------| SW       |
+-----------+             +----------+

Arduino Nano              Pan Servo          Tilt Servo
+-----------+             +----------+       +----------+
|        5V |-------------| VCC      |-------| VCC      |
|       GND |-------------| GND      |-------| GND      |
|        D9 |-------------| signal   |       |          |
|       D10 |----------------------------------| signal |
+-----------+             +----------+       +----------+
```

- If both servos twitch or reset when moving together, the Nano's onboard 5V may be under strain — power the servos from a separate 5V supply with grounds tied to the Nano.
- Mount the pan servo at the base of the bracket and the tilt servo on the rotating pan platform, per your bracket kit's assembly instructions.
- `SW` on most joystick modules is active-LOW with an internal pull-up expectation — this sketch configures `INPUT_PULLUP` in software, so no external pull-up resistor is required.
