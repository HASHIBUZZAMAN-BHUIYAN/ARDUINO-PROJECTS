# Wiring Notes — Vibration FFT Predictive Maintenance Monitor

```
Uno              ADXL345 (I2C)        DS3231 RTC (I2C)
+--------+      +-----------+        +-----------+
|     A4 |------| SDA       |--------| SDA       |
|     A5 |------| SCL       |--------| SCL       |
|     5V |------| VCC       |   5V --| VCC       |
|    GND |------| GND       |  GND --| GND       |
+--------+      +-----------+        +-----------+

Uno              Relay module      Buzzer      Reset button
+--------+      +------------+    +--------+   +------------------+
|     D4 |------| IN         |    |        |   | one leg -> D2    |
|     D5 |---------------------- -| +      |   | other leg -> GND |
|     D2 |----------------------------------- -| (INPUT_PULLUP)   |
+--------+      +------------+    +--------+   +------------------+

Uno              SD module (hw SPI)
+--------+      +------------+
|    D10 |------| CS         |
|    D11 |------| MOSI       |
|    D12 |------| MISO       |
|    D13 |------| SCK        |
+--------+      +------------+
```

- The ADXL345 must be mounted rigidly and directly to the machine housing (screwed or strongly adhered) — anything loose or foam-mounted damps the very vibration you're trying to measure.
- Keep I2C wiring runs short; at the sample rates used here, a noisy/long I2C bus can cause occasional dropped reads that show up as spectral artifacts.
- The reset button uses `INPUT_PULLUP`, so it only needs one leg wired to a digital pin and the other to GND — no external pull-up resistor needed.
