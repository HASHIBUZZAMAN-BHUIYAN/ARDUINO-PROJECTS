# RFID Door Lock

Scan an authorized RFID card/fob at an MFRC522 reader to swing a servo-driven lock bolt open for a few seconds; unrecognized cards trigger a red-LED/buzzer reject.

## Difficulty & Board

**Tier:** Intermediate
**Board:** Arduino Nano

Reasoning: the MFRC522 uses SPI (4 pins) plus a servo and two indicator outputs — modest pin count, and this is meant to be built into a small enclosure mounted next to a door, where the Nano's compact size is a practical advantage over a full-size Uno.

## Components

| Part | Qty |
|---|---|
| Arduino Nano | 1 |
| MFRC522 RFID reader module + card/fob | 1 |
| SG90 micro servo (acts as the bolt actuator) | 1 |
| Green LED | 1 |
| Red LED | 1 |
| 220 Ω resistor | 2 |
| Piezo buzzer | 1 |
| Breadboard | 1 |
| Jumper wires | ~12 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| 3V3 | MFRC522 VCC | MFRC522 is a 3.3V-only module — do not power it from 5V |
| GND | MFRC522 GND, both LED cathodes, buzzer - | shared ground |
| D10 | MFRC522 SDA (SS) | |
| D13 | MFRC522 SCK | shared SPI clock |
| D11 | MFRC522 MOSI | shared SPI data out |
| D12 | MFRC522 MISO | shared SPI data in |
| D9 | MFRC522 RST | |
| D5 | Lock servo signal | |
| D6 | Green LED via 220 Ω | access granted |
| D7 | Red LED via 220 Ω | access denied |
| D8 | Buzzer + | short beep on denial |

## How It Works

The MFRC522 talks to the Nano over SPI and, when a card enters its field, returns that card's unique ID (UID) — a short byte array. The sketch keeps a small hard-coded list of authorized UIDs (read once from Serial Monitor during setup to learn your own cards' IDs, then pasted into the code).

On every scan:

1. Read the UID bytes from the new card.
2. Compare them byte-for-byte against each entry in the authorized list.
3. **Match:** turn the green LED on, sweep the servo to the "unlocked" angle, hold for a few seconds, then sweep back to "locked" and turn the LED off.
4. **No match:** flash the red LED and beep the buzzer briefly.

This introduces reading structured data off an SPI peripheral (rather than a simple analog/digital pin) and comparing byte arrays — the core new concept versus earlier single-sensor projects.

## Setup & Flashing

1. Wire the MFRC522, servo, LEDs, and buzzer as above. **MFRC522 requires 3.3V power** — the Nano's 3V3 pin can supply it, but confirm your specific Nano clone's 3V3 regulator has enough headroom (a dedicated 3.3V supply is safer if you see erratic reads).
2. Install the **MFRC522** library (by GithubCommunity / miguelbalboa) via Library Manager.
3. First, upload the sketch with `LEARNING_MODE` left `true` (default) and open the Serial Monitor at 9600 baud — scan each card/fob you want to authorize and copy its printed UID.
4. Paste those UIDs into the `AUTHORIZED_UIDS` array, set `LEARNING_MODE` to `false`, and re-upload.
5. Test with both an authorized and an unauthorized card to confirm the correct LED/servo/buzzer response for each.

## Extensions

- Log every scan attempt (UID + granted/denied + timestamp) to an SD card (see the advanced-tier RFID + keypad access logger for a fuller version of this).
- Add a keypad PIN as a second factor required alongside the card.
- Store authorized UIDs in EEPROM instead of hard-coded in the sketch, with a "programming mode" button to add new cards without reflashing.
