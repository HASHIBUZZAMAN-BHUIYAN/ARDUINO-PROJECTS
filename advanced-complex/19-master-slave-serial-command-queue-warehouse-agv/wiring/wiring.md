# Wiring Notes — Warehouse AGV Dispatch

```
Dispatch Console (Uno)        4x4 Keypad          16x2 LCD
+--------+                   +------------+      +------------------------+
| D2-D9  |-------------------| rows/cols  |       | RS->D10 EN->D11        |
|        |                   +------------+       | D4->D12 D5->D13        |
|        |                                          | D6->A0  D7->A1         |
+--------+                                          +------------------------+

Dispatch Console (Uno)        AGV (Mega)
+--------+                   +--------+
| A2 (RX)|------------------>| D16(TX2)|
| A3 (TX)|<------------------| D17(RX2)|
| GND    |------------------>| GND     |
+--------+                   +--------+

AGV (Mega)                    IR line array (5 sensors)     L298N motor driver
+--------+                    +--------------------------+ +----------------+
| A8-A12 |--------------------| S1-S5                     | |                |
|  D8-11 |------------------------------------------------- -| IN1-IN4       |
+--------+                    +--------------------------+ +----------------+
```

- The console's `SoftwareSerial` link uses A2/A3 (used as digital pins here) crossed to the Mega's hardware `Serial2`, keeping both boards' USB `Serial` free for debugging.
- Space the IR array's 5 sensors evenly across the track width so the line-position error can be computed as a weighted average of which sensors see the line.
- Mark each station with a distinguishing pattern (e.g. a short perpendicular cross-line) wide enough for the array to reliably detect as "all sensors dark" versus the normal single-line-follow pattern.
