# Wiring Notes — LCD Menu System

```
Arduino Uno                16x2 LCD
+-----------+              +----------+
|        D7 |--------------| RS       |
|        D8 |--------------| EN       |
|        D9 |--------------| D4       |
|       D10 |--------------| D5       |
|       D11 |--------------| D6       |
|       D12 |--------------| D7       |
|        D6 |--------------| LED+ (backlight, PWM brightness) |
+-----------+              +----------+
   Contrast pot wiper -> LCD V0; pot outer legs -> 5V / GND
   LCD R/W -> GND

Arduino Uno                Rotary Encoder
+-----------+              +----------+
|        A0 |--------------| CLK      |
|        A1 |--------------| DT       |
|        A2 |--------------| SW       |
|        5V |--------------| +        |
|       GND |--------------| GND      |
+-----------+              +----------+

Arduino Uno                3-Channel Relay Module
+-----------+              +----------+
|         D2|--------------| IN1      |
|         D3|--------------| IN2      |
|         D4|--------------| IN3      |
|        5V |--------------| VCC      |
|       GND |--------------| GND      |
+-----------+              +----------+
   Each relay's COM/NO -> an LED (+resistor) -> GND, standing in for a real device
```

- Most encoder breakout modules include their own pull-up resistors on CLK/DT/SW; if yours doesn't, enable the Uno's internal pull-ups in software (`INPUT_PULLUP`), which this sketch already does.
- If the backlight pin on your specific LCD module can't be PWM-dimmed directly (some tie it straight to 5V), drive it through an NPN transistor instead, with D6 controlling the transistor's base through a current-limiting resistor.
