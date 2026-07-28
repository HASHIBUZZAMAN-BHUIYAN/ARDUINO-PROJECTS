# Wiring Notes — Soil Moisture Plant Alert

```
Arduino Uno Q            Capacitive Soil Sensor
+-----------+           +----------------------+
|       3V3 |-----------| VCC                  |
|       GND |----+------| GND                  |
|        A0 |-----------| AOUT                 |
+-----------+    |      +----------------------+
                 |
     D5-[220ohm]-+----|>|--- LED (cathode to GND rail)
     D6 ----------------- Buzzer + (- to GND rail)
```

- Uno Q logic level is 3.3V. Use the 3V3 pin, not 5V, to power the sensor here.
- Keep the sensor's PCB (the green board, not the probe tip) out of the soil/water — only the probe prongs should be buried or dipped.
- Higher AOUT reading = drier soil (capacitive sensors are inverted relative to some resistive probes).
