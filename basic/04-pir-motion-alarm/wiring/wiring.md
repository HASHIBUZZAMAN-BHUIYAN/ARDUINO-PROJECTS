# Wiring Notes — PIR Motion Alarm

```
Arduino Uno              HC-SR501 PIR
+----------+            +-----------+
|       5V |------------| VCC       |
|      GND |----+-------| GND       |
|        D2|------------| OUT       |
+----------+    |       +-----------+
     D7 ---------+-----| Buzzer +   |
     D13-[220ohm]+-----|>|--- LED --+
                        (cathode to GND rail)
```

- PIR modules typically have two trimmer potentiometers on the board itself: one for sensitivity, one for output-hold time. Start with both centered.
- Some PIR boards have a jumper for "repeatable trigger" mode (H) vs "single trigger" mode (L) — use H (repeatable) so OUT stays HIGH continuously while motion is ongoing.
- Give the sensor 30-60 seconds after power-up to stabilize before testing.
