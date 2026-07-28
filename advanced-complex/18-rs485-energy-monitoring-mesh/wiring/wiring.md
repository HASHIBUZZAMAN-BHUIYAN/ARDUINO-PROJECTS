# Wiring Notes — RS-485 Energy Monitoring Mesh

```
                      Shared RS-485 bus (A, B, GND) - daisy chained
        +----------------+----------------+----------------+
        |                |                |                |
  +-----+-----+    +-----+-----+    +-----+-----+    +-----+-----+
  | MAX485    |    | MAX485    |    | MAX485    |    | MAX485    |
  | (Node 1)  |    | (Node 2)  |    | (Node 3)  |    | (Hub)     |
  +-----+-----+    +-----+-----+    +-----+-----+    +-----+-----+

Meter Node (Nano)        SCT-013 CT clamp        ZMPT101B voltage sensor
+--------+               +------------------+   +------------------------+
|     A0 |---------------| (via burden resistor)|
|     A1 |------------------------------------- --| AC output              |
+--------+               +------------------+   +------------------------+

Hub (Mega)                RTC (I2C)       SD (hw SPI)       W5500
+--------+                +----------+   +----------+      +--------+
| 20/21  |----------------| SDA/SCL  |
|     53 |------------------------------- --| CS       |
|     49 |----------------------------------------------- --| CS     |
+--------+                +----------+   +----------+      +--------+
```

- **Safety first:** the CT clamp itself is safe to install (it clips around the outside of an insulated wire, never contacting a live conductor), but the voltage-sensing side taps mains voltage directly through the ZMPT101B module — that connection must only be made by someone qualified, with the circuit de-energized during installation.
- Clamp the CT sensor around the LIVE wire only (not both live and neutral) — clamping around both cancels the magnetic field and reads zero.
- All 3 meter nodes' voltage sensors should reference the same mains phase as the hub's own supply for consistent phase-relationship calculations.
