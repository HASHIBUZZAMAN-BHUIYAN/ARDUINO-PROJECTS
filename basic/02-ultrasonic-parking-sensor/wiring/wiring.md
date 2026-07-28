# Wiring Notes — Ultrasonic Parking Sensor

```
Arduino Uno              HC-SR04
+----------+            +--------+
|       5V |------------| VCC    |
|      GND |----+-------| GND    |
|        D9|------------| TRIG   |
|       D10|------------| ECHO   |
+----------+    |       +--------+
                |
                |        Buzzer
                +--------| -     |
        D6 -------------| +     |
```

- HC-SR04 is a 5V sensor; VCC must go to the Uno's 5V pin, not 3.3V.
- ECHO outputs a 5V pulse, which is safe to read directly on the Uno (no level shifting needed since the Uno's logic is also 5V).
- Mount the sensor facing the direction you want to measure, with a clear, mostly-flat surface in front of it for best readings.
