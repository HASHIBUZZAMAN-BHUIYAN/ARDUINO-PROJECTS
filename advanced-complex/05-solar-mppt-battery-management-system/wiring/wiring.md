# Wiring Notes — Solar MPPT Battery Management System

```
Solar Panel --> INA219 #1 (addr 0x40) --> Buck converter input
                                                |
                                          [MOSFET switched
                                           by D9 PWM via
                                           gate driver]
                                                |
                                                v
                Battery <-- INA219 #2 (addr 0x41) <-- Buck converter output

Uno Q            INA219 #1        INA219 #2         SD          W5500
+--------+      +----------+     +----------+      +------+    +------+
| SDA/SCL|------| SDA/SCL  |-----| SDA/SCL  |       |      |    |      |
|     D9 |------| (PWM to gate driver, not to INA219)      |    |      |
|     53 |---------------------------------------------- --| CS |    |      |
|     49 |------------------------------------------------------- ---| CS   |
| 50/51/52 (shared SPI) ---------------------------------- -| MISO/MOSI/SCK together |
+--------+                                                +------+    +------+
```

- INA219 modules default to I2C address 0x40; set the second module's address pins (per its datasheet, usually solder jumpers) to 0x41 so both can share the bus.
- The buck converter's MOSFET gate should be driven through a proper gate driver or at minimum a gate resistor — driving a power MOSFET gate directly from an Arduino pin with no series resistor risks ringing and damage.
- Keep the high-current panel/battery/inductor wiring physically separate from the INA219's I2C signal wiring to avoid switching noise corrupting readings.
