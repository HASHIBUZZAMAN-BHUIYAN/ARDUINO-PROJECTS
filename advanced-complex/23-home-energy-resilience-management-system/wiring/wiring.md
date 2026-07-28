# Wiring Notes — Home Energy Resilience Management System

```
Grid leg --> INA219 #1 (0x40) --+
Solar leg --> INA219 #2 (0x41) --+--> Mega I2C (20/21)
Battery leg --> INA219 #3 (0x44) --+       |
                                          RTC (0x68, same bus)

Mega                 4-ch relay module
+--------+          +----------------------------------------+
|    D30 |----------| Transfer: Grid-in                        |
|    D31 |----------| Transfer: Solar/Battery-in                 |
|    D32 |----------| Load shed: priority 1 (non-critical)         |
|    D33 |----------| Load shed: priority 2 (non-critical)          |
+--------+          +----------------------------------------+

Mega                 SD (hw SPI)         W5500 (shares SPI)
+--------+          +------------+      +------------+
|     53 |----------| CS         |      |            |
|     49 |----------------------------- --| CS         |
| 50/51/52 (shared MISO/MOSI/SCK) --------+------------+
+--------+
```

- Set each INA219's address pins so all 3 sit at distinct addresses (0x40, 0x41, 0x44 per their datasheet's solder-jumper table) on the shared bus.
- The transfer relay pair (D30/D31) is wired to be mutually exclusive in software — never energize both simultaneously; a production build should back this with a hardware interlock as well.
- Load-shed relays (D32/D33) control non-critical circuit contactors, not the main service panel — this project manages a sub-panel of shed-able loads, not full-house switching.
