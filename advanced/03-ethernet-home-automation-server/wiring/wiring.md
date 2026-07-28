# Wiring Notes — Ethernet Home Automation Server

```
Ethernet shield stacks directly onto the Uno Q's headers (Uno form factor).
No manual wiring is needed for the shield itself; it uses:
  - SPI bus (pins 11, 12, 13) for W5100/W5500 communication
  - Pin 10 as the Ethernet chip-select (CS)

Arduino Uno Q             4-Channel Relay Module
+-----------+             +----------+
|         D2|-------------| IN1      | (Lamp)
|         D3|-------------| IN2      | (Fan)
|         D4|-------------| IN3      | (Heater)
|         D5|-------------| IN4      | (Outlet)
|        5V |-------------| VCC      |
|       GND |-------------| GND      |
+-----------+             +----------+
   Each relay COM/NO -> an LED (+resistor) -> GND, standing in for a real appliance
```

- Since the shield occupies pins 10-13 for its own SPI/CS use, avoid wiring anything else to those pins — the relay module here intentionally uses D2-D5 instead.
- Confirm your Ethernet shield's chip-select pin: most W5100/W5500 shields default to pin 10, matching the `Ethernet.init(10)` call in the sketch; adjust if your shield's silkscreen documents a different pin.
- If your router's DHCP range overlaps the static IP chosen in the sketch, either pick an address outside that range or switch the sketch to `Ethernet.begin(mac)` for automatic DHCP assignment.
