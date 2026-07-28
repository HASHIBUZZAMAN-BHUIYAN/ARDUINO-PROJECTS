# Bluetooth Smart Lamp

An HC-05 Bluetooth module lets a phone's Bluetooth terminal app send simple text commands to change an RGB LED strip's color, brightness, and a couple of built-in lighting effects (fade, strobe, off).

## Difficulty & Board

**Tier:** Intermediate
**Board:** Arduino Nano

Reasoning: an HC-05 (serial, 2 pins) plus 3 PWM channels for RGB control is a small, self-contained pin count for a lamp base that the Nano's compact size suits well.

## Components

| Part | Qty |
|---|---|
| Arduino Nano | 1 |
| HC-05 Bluetooth serial module | 1 |
| Common-cathode RGB LED strip (short segment) or single RGB LED | 1 |
| N-channel MOSFET (e.g. IRLZ44N) | 3 |
| 220 Ω resistor (MOSFET gate) | 3 |
| 10 kΩ resistor (voltage divider for HC-05 RX) | 1 |
| 20 kΩ resistor (voltage divider for HC-05 RX) | 1 |
| Breadboard | 1 |
| Jumper wires | ~12 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| 5V | HC-05 VCC | |
| GND | HC-05 GND, MOSFET sources, strip GND | shared ground |
| D10 (TX) | HC-05 RXD | wire directly, Nano TX (5V) into HC-05 RX is commonly tolerated on most HC-05 boards, but see wiring notes for the safer divider option |
| D11 (RX) | HC-05 TXD via 10k/20k divider | HC-05 TX is 3.3V and safe straight into the Nano, but a divider is included for boards with a different logic level |
| D5 (PWM) | Red channel MOSFET gate via 220 Ω | |
| D6 (PWM) | Green channel MOSFET gate via 220 Ω | |
| D9 (PWM) | Blue channel MOSFET gate via 220 Ω | |

## How It Works

`SoftwareSerial` on D10/D11 talks to the HC-05 at its default 9600 baud, separate from the USB serial connection used for programming/debugging. Any Bluetooth terminal app on a phone (there are many free ones) can pair with the HC-05 and send plain text lines, which the sketch parses as simple commands:

- `RGB,255,0,128` — sets red/green/blue channels directly (0-255 each).
- `BRIGHT,150` — scales overall brightness without changing the current hue.
- `FADE` — starts a continuous smooth color-cycling effect.
- `STROBE` — starts a fast on/off white flash effect.
- `OFF` — turns all channels off.

Commands are read a line at a time (terminated by `\n`, which most terminal apps send automatically on "send"), split on commas, and dispatched with a simple string-prefix check. This project's core concept is a tiny text-based protocol over serial — the same pattern scales directly to WiFi/MQTT-based smart home commands, just swapping the transport.

## Setup & Flashing

1. Wire the HC-05 and RGB MOSFET driver stage as above.
2. No external Arduino library is required (`SoftwareSerial` is bundled with the IDE).
3. Open `src/bluetooth_lamp.ino` in the Arduino IDE.
4. Select **Tools > Board > Arduino Nano** (correct processor/bootloader for your clone) and the correct COM port.
5. **Disconnect the HC-05's RX/TX wires before uploading** — having the module attached to D10/D11 during upload doesn't usually conflict since those aren't the hardware UART pins, but it's good practice to double check if upload fails.
6. Upload, then pair your phone with the HC-05 (default PIN is often `1234` or `0000`), open a Bluetooth terminal app, and send commands like `RGB,255,0,0` to test.

## Extensions

- Add preset "scenes" (e.g. `SCENE,SUNSET`) that ramp through a sequence of colors automatically.
- Replace Bluetooth with the advanced-tier ESP8266 WiFi pattern to control the lamp from anywhere on the home network instead of only within Bluetooth range.
- Add a physical button as a manual on/off override alongside the Bluetooth control.
