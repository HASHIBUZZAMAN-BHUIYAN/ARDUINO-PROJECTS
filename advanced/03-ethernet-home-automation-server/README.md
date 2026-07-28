# Ethernet Home Automation Server

The board hosts its own small web page (via a wired Ethernet shield) listing four relay-controlled "devices" with ON/OFF links — visit its IP address from any browser on the home network to toggle lamps/appliances, no app or cloud account required.

## Difficulty & Board

**Tier:** Advanced
**Board:** Arduino Uno Q

Reasoning: this project needs networking, and a wired Ethernet shield is the most reliable, best-documented way to get an Arduino sketch onto the network using the standard, rock-solid `Ethernet` library — it stacks directly onto the Uno Q's Uno-form-factor headers. Wired Ethernet also sidesteps any uncertainty around a specific board's WiFi stack, keeping this sketch broadly portable. (The repo's other IoT-style advanced project uses WiFi instead, via an ESP8266 co-processor — see `advanced/07-esp8266-wifi-weather-telemetry`.)

## Components

| Part | Qty |
|---|---|
| Arduino Uno Q | 1 |
| W5100 or W5500 Ethernet shield | 1 |
| Ethernet cable + router/switch port | 1 |
| 4-channel relay module | 1 |
| LED (stands in for each controlled appliance) | 4 |
| Jumper wires | ~10 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| Shield stacks directly on the header pins | Ethernet shield | uses SPI (pins 11-13) + pin 10 (CS) on Uno-footprint boards — no manual wiring needed for the shield itself |
| D2 | Relay 1 IN | Device "Lamp" |
| D3 | Relay 2 IN | Device "Fan" |
| D4 | Relay 3 IN | Device "Heater" |
| D5 | Relay 4 IN | Device "Outlet" |
| 5V | Relay module VCC | |
| GND | Relay module GND | shared ground, also shared with the Ethernet shield (automatic via the stacked header) |

## How It Works

The `Ethernet` library brings up a minimal web server on port 80 using a fixed local IP address (set to match your home network's subnet — adjust `ip[]` in the sketch if `192.168.1.177` isn't free on yours). Every time a browser connects, the sketch:

1. Reads the raw HTTP request line looking for a `GET /toggle?d=N` pattern, where `N` is a device index 0-3.
2. If found, flips that relay's state.
3. Regardless of whether a toggle happened, responds with a freshly-generated HTML page listing all four devices' current ON/OFF state, each with a link that requests `/toggle?d=N` for that device — clicking a link is itself a fresh page load/request, which is why no JavaScript is needed for this simple case.

This is the "networking" advanced concept: parsing enough of raw HTTP to build a tiny control panel, and understanding that (unlike the Bluetooth or IR projects) this device is now reachable by literally anything on the local network, which also means basic security awareness matters even for a toy project like this (no authentication is implemented here — treat this as trusted-network-only, not internet-facing).

## Setup & Flashing

1. Stack the Ethernet shield onto the Uno Q and wire the relay module as above.
2. No external library install is required — `Ethernet` and `SPI` both ship with the Arduino IDE.
3. Open `src/ethernet_home_server.ino`, and set the `mac[]` array (any locally-administered MAC works, e.g. leave the placeholder) and the `ip[]` array to an address on your LAN's subnet that's outside your router's DHCP range.
4. Select **Tools > Board > Arduino Uno Q** and the correct COM port, then upload.
5. Connect an Ethernet cable from the shield to your router, open the Serial Monitor at 9600 baud to confirm the assigned IP printed on boot, then visit `http://<that IP>/` from a browser on the same network and click the toggle links.

## Extensions

- Add basic HTTP authentication (a simple shared-secret query parameter check) before allowing toggles.
- Replace the fixed IP with `Ethernet.begin(mac)` (DHCP) plus mDNS so you don't need to hard-code an address.
- Combine with the intermediate IR remote hub so the same relays can be controlled from either the network or a physical remote.
