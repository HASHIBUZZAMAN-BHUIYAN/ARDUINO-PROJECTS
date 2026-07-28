# Wiring Notes — Audio Spectrum LED Matrix Visualizer

```
Uno              Mic module         Sensitivity pot     RTC (I2C)
+--------+      +------------+     +--------------+    +----------+
|     A0 |------| AOUT       |     |              |     |          |
|     A1 |---------------------- --| wiper        |     |          |
| A4/A5  |------------------------------------------- --| SDA/SCL  |
+--------+      +------------+     +--------------+    +----------+

Uno              MAX7219 matrix        SD module (shares SPI)
+--------+      +----------------+    +--------------+
|    D10 |------| CS             |    |              |
|    D11 |------| DIN            |    |              |
|    D13 |------| CLK            |    |              |
|     D4 |------------------------- --| CS           |
|    D11 |------------------------- --| MOSI (shared)|
|    D12 |------------------------- --| MISO         |
|    D13 |------------------------- --| SCK (shared) |
+--------+      +----------------+    +--------------+
```

- MAX7219 uses software SPI-like signaling via `LedControl` (DIN/CLK/CS pins named individually, not the hardware SPI bus) so it can coexist on the same physical pins the SD card's hardware SPI also uses, as long as CS lines are distinct and only one device is addressed at a time.
- Keep the microphone module's analog wiring short and away from the matrix's switching lines to minimize induced noise on A0.
- The sensitivity pot's outer legs go to 5V and GND; only the wiper connects to A1.
