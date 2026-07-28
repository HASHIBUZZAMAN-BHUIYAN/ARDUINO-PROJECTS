# Wiring Notes — Bluetooth Smart Lamp

```
Arduino Nano              HC-05 Bluetooth
+-----------+             +----------+
|        5V |-------------| VCC      |
|       GND |----+--------| GND      |
|       D10 |-------------| RXD      |  (Nano TX -> HC-05 RX)
|       D11 |--[10k]--+---| TXD      |  (HC-05 TX -> Nano RX, via divider)
+-----------+          |
                       [20k]
                        |
                       GND
```

```
Arduino Nano              MOSFET stage (x3, one per color channel)
+-----------+             +---------------------------+
|        D5 |--[220ohm]---| Gate  (Red channel)        |
|        D6 |--[220ohm]---| Gate  (Green channel)      |
|        D9 |--[220ohm]---| Gate  (Blue channel)       |
+-----------+             +---------------------------+
   Each MOSFET: Source -> GND, Drain -> corresponding RGB strip channel wire
   RGB strip's common (+) wire -> external 12V/5V supply matching the strip
```

- The resistor divider on D11 steps the HC-05's TX signal down before it reaches the Nano's RX pin; many HC-05 boards output 3.3V logic already and this divider is optional in that case, but it's cheap insurance.
- If your RGB strip needs more current than the Nano can source directly, power the strip from its own supply (matching its voltage) and only switch it through the MOSFETs — the Nano's 5V pin should not power the strip directly.
- Common-cathode strips/LEDs: all channel cathodes tie to the shared MOSFET-switched ground. If using a common-anode strip instead, the driving logic needs to be inverted.
