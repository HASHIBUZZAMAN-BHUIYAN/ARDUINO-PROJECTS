# Wiring Notes — Joystick Robot Arm

```
Arduino Mega              2-Axis Joystick
+-----------+             +----------+
|        5V |-------------| VCC      |
|       GND |-------------| GND      |
|        A0 |-------------| VRx      |
|        A1 |-------------| VRy      |
|       D22 |-------------| SW       |
+-----------+             +----------+

     D23 --- Elbow-up button    --- other leg to GND (INPUT_PULLUP in sketch)
     D24 --- Elbow-down button  --- other leg to GND
     D25 --- Record-pose button --- other leg to GND
     D26 --- Play-sequence button --- other leg to GND

External 5V/2A+ supply        Servos (x4: base, shoulder, elbow, gripper)
+-----------+                 +----------+
|         + |-----------------| VCC (all 4, shared) |
|         - |-----------------| GND (all 4, shared, tied to Mega GND too) |
+-----------+                 +----------+

     D9  -> Base servo signal
     D10 -> Shoulder servo signal
     D11 -> Elbow servo signal
     D12 -> Gripper servo signal
```

- All 4 servos should share one external power rail sized for at least 2A — driving several servos at once from the Mega's onboard 5V regulator can brown out the board.
- Tie the external supply's ground to the Mega's GND even though they don't share the same power source, so the PWM signal reference is consistent.
- All four button pins use the Mega's internal pull-ups (`INPUT_PULLUP`) — wire the free leg of each button straight to GND, no external resistor needed.
