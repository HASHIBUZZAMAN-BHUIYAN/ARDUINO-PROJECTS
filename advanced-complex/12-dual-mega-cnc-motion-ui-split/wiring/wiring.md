# Wiring Notes — Dual-Board CNC Motion/UI Split

```
Mega (Motion Controller)                       Uno (UI Controller)
+------------------+                          +------------------+
| D16 (TX2)         |------------------------>| D4 (RX)           |
| D17 (RX2)          |<------------------------| D10 (TX)          |
| GND                |------------------------| GND               |
+------------------+                          +------------------+

Mega              A4988 x3 (STEP/DIR)      Limit switches (x3)     Spindle relay
+--------+       +------------------+     +--------------------+ +------------+
|D22/24/26|-------| STEP x3          |     | X: D2 (INT)         | | D28 -> IN  |
|D23/25/27|-------| DIR x3           |     | Y: D3 (INT)         | |            |
+--------+       +------------------+     | Z: D18 (INT)        | +------------+
                                            +--------------------+

Uno               SD (hw SPI)         16x2 LCD
+--------+       +------------+      +------------------------+
|     D9 |-------| CS         |      | RS->D2  EN->D3          |
|    D11 |-------| MOSI       |      | D4->D5 D5->D6 D6->D7 D7->D8 |
|    D12 |-------| MISO       |      +------------------------+
|    D13 |-------| SCK        |
+--------+       +------------+
```

- The two boards' UART lines are crossed (Mega TX2 -> Uno RX pin, Mega RX2 -> Uno TX pin) and must share a common ground for reliable signaling.
- `Serial2` on the Mega is a separate hardware UART from the USB-connected `Serial`, so you can keep a USB debug console open on the Mega while it talks to the Uno.
- Set each A4988's current-limit trimpot before first use, as in the single-board CNC project in this tier.
