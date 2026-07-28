# Wiring Notes — Light-Activated Night Light

```
                 Nano 5V
                    |
                   LDR
                    |
                    +---------- A0 (divider midpoint)
                    |
                  10k ohm
                    |
                 Nano GND

Nano D9 --[220 ohm]-->|-- LED --GND
```

- The LDR + 10k resistor form a voltage divider. Swapping their order (LDR to GND, resistor to 5V, tap still at the midpoint) inverts the reading direction — if your thresholds seem backwards, check which way you wired it.
- D9 is used for the LED because it supports `analogWrite()` (PWM) for the fade-in effect.
