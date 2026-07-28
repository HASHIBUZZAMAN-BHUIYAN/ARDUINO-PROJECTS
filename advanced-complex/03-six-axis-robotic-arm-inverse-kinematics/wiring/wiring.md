# Wiring Notes — Six-Axis Robotic Arm

```
                External 5V/5A supply
                +--------+
                |  V+  G |
                +---+--+-+
                    |  |
     +--------------+  +-------------------+
     |                                      |
     v                                      v
 Servo rail (all 6 servo V+ / GND)     Uno GND (common ground - required)

Uno            Servos (signal only, power from rail above)
+--------+    +-------------------------------------------+
|     D3 |----| Base                                       |
|     D5 |----| Shoulder                                   |
|     D6 |----| Elbow (also has geared feedback pot)        |
|     D9 |----| Wrist pitch                                 |
|    D10 |----| Wrist roll                                  |
|    D11 |----| Gripper                                     |
|     A0 |----| Elbow feedback pot wiper                    |
+--------+    +-------------------------------------------+
```

- All 6 servos share one external 5V/5A supply; the Uno's onboard regulator cannot supply this much current. The Uno's GND must still be tied to the supply's GND so PWM signal timing has a common reference.
- The elbow feedback potentiometer is mechanically geared 1:1 to the elbow joint (e.g. via a small gear or a second linkage arm) — it does NOT sit inside the servo, it's an external sensor added purely for this project's closed-loop check.
- Route servo signal wires away from the supply's power leads to reduce PWM jitter from induced noise.
