# Dual-Board Wireless Security System

Two Arduinos talk to each other over a pair of nRF24L01 radios: a battery-friendly remote sensor node (PIR + door reed switch) watches a room and wirelessly reports events to a base station, which sounds a siren, shows status on an LCD, and displays a live "last seen" signal-health check.

## Difficulty & Board

**Tier:** Advanced
**Board:** Arduino Nano (remote sensor node) + Arduino Uno (base station)

Reasoning: this is the repo's "multiple boards talking to each other" project. The remote node is battery-powered and tucked into a room out of sight, so the compact Nano is the natural fit there; the base station sits near an outlet with an LCD and siren and has a slightly larger, standard pin layout, for which the default Uno was used.

## Components

| Part | Qty |
|---|---|
| Arduino Nano (remote node) | 1 |
| Arduino Uno (base station) | 1 |
| nRF24L01+ radio module | 2 |
| HC-SR501 PIR motion sensor | 1 |
| Magnetic reed switch | 1 |
| 16x2 character LCD | 1 |
| 10 kΩ potentiometer (LCD contrast) | 1 |
| Piezo buzzer/siren | 1 |
| 10 kΩ resistor (reed switch pull-down) | 1 |
| Battery pack (remote node power) | 1 |
| Breadboard x2 | 2 |
| Jumper wires | ~24 |

## Wiring — Remote Sensor Node (Nano)

| Arduino Pin | Component | Notes |
|---|---|---|
| 3V3 | nRF24L01 VCC | nRF24L01 is a 3.3V-only radio |
| GND | nRF24L01 GND, PIR GND, reed switch GND-side | shared ground |
| D9 | nRF24L01 CE | |
| D10 | nRF24L01 CSN | |
| D13 | nRF24L01 SCK | hardware SPI |
| D11 | nRF24L01 MOSI | hardware SPI |
| D12 | nRF24L01 MISO | hardware SPI |
| D2 | PIR OUT | |
| A0 | Reed switch | with 10kΩ pull-down to GND |

## Wiring — Base Station (Uno)

| Arduino Pin | Component | Notes |
|---|---|---|
| 3V3 | nRF24L01 VCC | |
| GND | nRF24L01 GND, LCD VSS, buzzer - | shared ground |
| D9 | nRF24L01 CE | |
| D10 | nRF24L01 CSN | |
| D13 | nRF24L01 SCK | hardware SPI |
| D11 | nRF24L01 MOSI | hardware SPI |
| D12 | nRF24L01 MISO | hardware SPI |
| D2-D7 | LCD RS, EN, D4-D7 | |
| D8 | Buzzer + | |

## How It Works

The `RF24` library wraps the nRF24L01's packet radio into a simple `write()`/`read()` API over a fixed 5-byte "pipe address" both boards agree on in code. The remote node polls its two sensors continuously; whenever either one's state changes, it packs a tiny struct (`{sensorType, state, sequenceNumber}`) and transmits it. It also sends a lightweight periodic "heartbeat" packet every 10 seconds even with no sensor change, purely so the base station can tell "no alerts" apart from "the remote node lost power/went out of range" — a critical distinction for any real security system.

The base station listens continuously, and:

- On a PIR or door event packet, it lights up the LCD with which zone triggered and sounds the siren.
- On a heartbeat packet, it just resets an internal "last heard from remote" timer.
- If more than ~25 seconds pass without *any* packet (heartbeat or event), it shows a "REMOTE OFFLINE" warning on the LCD — treating radio silence itself as an alertable condition, not just sensor trips.

This project's core new concept versus the intermediate two-zone alarm is that the sensing and the response/UI are now on two physically separate, wirelessly-linked boards, plus the "heartbeat" pattern for detecting a dead/out-of-range link.

## Setup & Flashing

1. Wire the remote node (Nano) and base station (Uno) as their separate tables above — note both use the *same* CE/CSN/SPI pin numbers, they're just two different physical boards.
2. Install the **RF24** library (by TMRh20) via Library Manager on both boards' project (it's the same library either way).
3. Open `src/remote_node.ino`, select **Tools > Board > Arduino Nano**, select the Nano's COM port, and upload it to the remote board.
4. Open `src/base_station.ino`, select **Tools > Board > Arduino Uno**, select the Uno's COM port, and upload it to the base board.
5. Power both boards. Open the base station's Serial Monitor at 9600 baud; trigger the PIR or open/close the reed switch on the remote node and confirm the base station's LCD and siren respond within a second or two.
6. Power off the remote node and confirm the base station shows "REMOTE OFFLINE" after about 25 seconds.

## Extensions

- Add more remote nodes (each with a distinct ID in its packets) covering different rooms, all reporting to the same base station.
- Add an acknowledgment/retry check using `RF24`'s built-in auto-ack feature to detect and log dropped packets explicitly.
- Give the remote node a sleep mode (waking only on PIR interrupt) to dramatically extend battery life between events.
