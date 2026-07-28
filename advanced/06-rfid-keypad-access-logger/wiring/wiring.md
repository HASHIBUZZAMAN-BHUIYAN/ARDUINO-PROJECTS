# Wiring Notes — RFID + Keypad Access Control Logger

```
Arduino Mega               MFRC522                     microSD module
+-----------+              +----------+                +----------+
|       3V3 |--------------| VCC      |                |          |
|       GND |----+---------| GND      |----+-----------| GND      |
|       D48 |--------------| SDA(SS)  |    |            |          |
|       D49 |--------------| RST      |    |            |          |
|       D50 |--------------| MISO     |----+------------| MISO     |
|       D51 |--------------| MOSI     |----+------------| MOSI     |
|       D52 |--------------| SCK      |----+------------| SCK      |
|       D53 |------------------------------------------ | CS       |
+-----------+              +----------+                +----------+
   (MFRC522 and SD share the same hardware SPI bus (50/51/52); each has
    its own chip-select line: D48 for the reader, D53 for the SD card.)

Arduino Mega               4x4 Matrix Keypad
+-----------+              +--------------------+
|        A8 |--------------| Row 1              |
|        A9 |--------------| Row 2              |
|       A10 |--------------| Row 3              |
|       A11 |--------------| Row 4              |
|       A12 |--------------| Col 1              |
|       A13 |--------------| Col 2              |
|       A14 |--------------| Col 3              |
|       A15 |--------------| Col 4              |
+-----------+              +--------------------+

Arduino Mega               DS3231 RTC (I2C)          16x2 LCD
+-----------+              +----------+             +----------+
|         20|--------------| SDA      |    D22 ------| RS       |
|         21|--------------| SCL      |    D23 ------| EN       |
+-----------+              +----------+    D24 ------| D4       |
                                            D25 ------| D5       |
                                            D26 ------| D6       |
                                            D27 ------| D7       |
                                            +----------+
                            Contrast pot wiper -> LCD V0; pot outer legs -> 5V/GND

     D9 ----------------- Lock servo signal (VCC/GND to 5V/GND)
     D8 ----------------- Buzzer + (- to GND)
```

- Analog pins A8-A15 are used as plain digital I/O for the keypad here since the Mega's many analog pins can double as digital pins (`pinMode`/`digitalRead` work identically on them) — this frees up more of the lower digital pins for other peripherals.
- MFRC522 and the SD module share the same physical SPI bus (MISO/MOSI/SCK) because SPI is designed for multiple devices — only the CS/SS line needs to be unique per device, which is why the reader uses D48 while the SD card keeps the Mega's default D53.
