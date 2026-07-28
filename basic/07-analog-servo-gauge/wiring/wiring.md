# Wiring Notes — Analog Servo Gauge

```
Arduino Nano            10k Potentiometer
+-----------+          +-------------------+
|        5V |----------| outer pin 1       |
|       GND |----------| outer pin 2       |
|         A0|----------| wiper (middle)    |
+-----------+          +-------------------+

Arduino Nano            SG90 Servo
+-----------+          +-------------------+
|        5V |----------| VCC (red)         |
|       GND |----------| GND (brown/black) |
|         D9|----------| signal (orange)   |
+-----------+          +-------------------+
```

- If powering the servo from the Nano's onboard 5V regulator feels weak or the servo twitches under load, power it from an external 5V supply instead, with grounds tied together between the Nano and the external supply.
- Mount the servo horn, then attach a paper pointer/arrow so it's centered when the potentiometer sits at its midpoint.
