# Wiring Notes — Stepper Motor CNC Mini Plotter

```
Arduino Mega              A4988 (X axis)              NEMA17 (X)
+-----------+            +------------+              +----------+
|        D2 |------------| STEP       |              |          |
|        D3 |------------| DIR        |              |          |
|        D4 |------------| ENABLE     |              |          |
|        5V |------------| VDD        |   1A/1B,2A/2B | coil out |
|       GND |------------| GND (logic)|--------------|          |
+-----------+            | VMOT       |----- 12V supply +
                          | GND (motor)|----- 12V supply -
                          +------------+

Arduino Mega              A4988 (Y axis)              NEMA17 (Y)
+-----------+            +------------+              +----------+
|        D5 |------------| STEP       |              |          |
|        D6 |------------| DIR        |              |          |
|        D7 |------------| ENABLE     |              |          |
|        5V |------------| VDD        |              |          |
|       GND |------------| GND (logic)|--------------|          |
+-----------+            | VMOT       |----- 12V supply +
                          | GND (motor)|----- 12V supply -
                          +------------+

Arduino Mega              Pen-lift servo             Limit switches (optional)
+-----------+            +----------+               +---------------------+
|        5V |------------| VCC      |    D30 --------| X switch (to GND)   |
|       GND |------------| GND      |    D31 --------| Y switch (to GND)   |
|        D9 |------------| signal   |    (INPUT_PULLUP, no external resistor needed)
+-----------+            +----------+
```

- A4988 VMOT (motor power) must be a separate 12V supply from the Mega's own 5V logic supply, but their grounds must be tied together.
- Add a 100uF electrolytic capacitor across VMOT/GND close to each A4988 board — stepper drivers are prone to voltage spikes when motor current changes suddenly, and this protects the driver.
- Set each driver's current limit trimmer (`Vref = current_limit * sense_resistor`, consult your specific A4988 board's silkscreen/datasheet) before running motors continuously, to avoid overheating.
