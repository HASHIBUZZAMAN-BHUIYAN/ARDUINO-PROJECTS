# Wiring Notes — nRF24 Swarm Formation Robots

```
Leader (Uno)         nRF24L01           Left encoder      Right encoder
+--------+          +----------+       +------------+    +------------+
|     D9 |----------| CE       |   D2--| A (INT)     | D3-| A (INT)     |
|    D10 |----------| CSN      |   A0--| B           | A1-| B           |
| D11-13 |----------| SPI      |
+--------+          +----------+

Leader (Uno)         L298N motor driver (both wheels)
+--------+          +----------------------------+
| D4-D7  |----------| IN1-IN4                     |
+--------+          +----------------------------+

Follower (Nano)      nRF24L01           HC-SR04            L298N
+--------+          +----------+       +------------+     +----------+
|     D9 |----------| CE       |   D6--| TRIG        | D2-4| IN1-IN4  |
|    D10 |----------| CSN      |   D7--| ECHO        |
| D11-13 |----------| SPI      |
+--------+          +----------+       +------------+     +----------+
```

- All 3 nRF24L01 modules must use the same "channel" (RF24 channel number) and matching pipe addresses to talk to each other, but each follower's status-pipe address is distinct from the others'.
- Power nRF24L01 modules from 3.3V only; add a 10-100uF capacitor across VCC/GND on each if you see unreliable reception, per this repo's other nRF24-based project (`advanced/04`).
- Aim each follower's HC-SR04 at the leader's rear-facing flat surface for a consistent, reliable echo.
