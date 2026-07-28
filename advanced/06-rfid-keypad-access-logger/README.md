# RFID + Keypad Access Control Logger

A two-factor door control: scan an RFID card, then enter a 4-digit PIN on a keypad within a time limit. Every attempt — successful or not — is timestamped via RTC and appended to an SD card log, and an LCD shows live status.

## Difficulty & Board

**Tier:** Advanced
**Board:** Arduino Mega 2560

Reasoning: this project stacks an MFRC522 reader (SPI, several pins), a 4x4 keypad (8 pins), a 16x2 LCD (6 pins), an RTC (I2C), an SD module (SPI, 4 pins), a lock servo, and a status LED/buzzer — well over 20 I/O lines in total. That's squarely the "pin-heavy multi-component security system" case this repo favors the Mega for, and it's a natural step up in complexity from the intermediate-tier single-factor RFID door lock.

## Components

| Part | Qty |
|---|---|
| Arduino Mega 2560 | 1 |
| MFRC522 RFID reader module + card/fob | 1 |
| 4x4 matrix keypad | 1 |
| 16x2 character LCD | 1 |
| 10 kΩ potentiometer (LCD contrast) | 1 |
| DS3231 RTC module (I2C) | 1 |
| microSD card module (SPI) + microSD card | 1 |
| SG90 micro servo (lock bolt) | 1 |
| Piezo buzzer | 1 |
| Breadboard / protoboard | 1 |
| Jumper wires | ~30 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| 3V3 | MFRC522 VCC | 3.3V-only module |
| D48 | MFRC522 SDA (SS) | using the Mega's non-default SPI SS pins so pin 53 stays free for the SD card |
| D49 | MFRC522 RST | |
| 50/51/52 | MFRC522 MISO/MOSI/SCK, SD MISO/MOSI/SCK | Mega hardware SPI bus, shared by both SPI devices (each with its own CS) |
| D53 | SD module CS (SS) | Mega's default hardware SS pin |
| A8-A15 | Keypad rows/columns (4+4) | using analog pins as extra digital I/O, common practice on the Mega when digital pins run short |
| 20 (SDA) | RTC SDA | Mega's dedicated I2C pins |
| 21 (SCL) | RTC SCL | |
| D22-D27 | LCD RS, EN, D4-D7 | |
| D9 | Lock servo signal | |
| D8 | Buzzer + | |

## How It Works

Access requires **two factors within a time window**: scanning a recognized RFID card starts a 10-second countdown (shown on the LCD) during which the correct 4-digit PIN must be entered on the keypad. Either factor alone is insufficient — a stolen card without the PIN, or a guessed/shoulder-surfed PIN without the card, both fail. This two-factor pattern is the core new concept versus the intermediate-tier RFID lock (card-only) and two-zone alarm (PIN-only) projects.

Every attempt is logged as a CSV row on the SD card via the RTC-supplied timestamp: the scanned card's UID, whether the PIN that followed was correct, and the final outcome (granted / denied / timed-out waiting for a PIN). This creates a full audit trail — who tried to get in and when, not just who succeeded — which is standard practice for any real access-control system and is what elevates this beyond a simple lock into a "logger."

On success, the servo swings to the unlocked position for a few seconds and the LCD shows a welcome message; on failure (wrong PIN, or timeout), the buzzer sounds a short reject tone and the attempt is still logged.

## Setup & Flashing

1. Wire the RFID reader, keypad, LCD, RTC, SD module, servo, and buzzer as above.
2. Install these libraries via Library Manager: `MFRC522`, `Keypad`, `RTClib`, plus the bundled `SD`/`SPI`/`Wire`/`LiquidCrystal`/`Servo`. See `libraries.txt`.
3. Format the microSD card as FAT32 before inserting it.
4. Open `src/access_logger.ino`. First upload with `LEARNING_MODE = true` and use the Serial Monitor at 9600 baud to scan your card(s) and note the printed UID(s); paste them into `AUTHORIZED_UIDS`, set `LEARNING_MODE = false`, and set your own `const char PIN_CODE[] = "..."`.
5. Select **Tools > Board > Arduino Mega or Mega 2560** and the correct COM port, then upload.
6. Test: scan an authorized card, enter the correct PIN within 10 seconds, and confirm the servo unlocks; then test a wrong PIN and a PIN timeout, and check `ACCESS.CSV` on the SD card afterward to confirm all three attempts were logged.

## Extensions

- Add a lockout that temporarily disables the keypad after 3 consecutive wrong-PIN attempts, to slow down brute-force guessing.
- Support multiple PINs, one per authorized card, instead of one shared PIN for everyone.
- Add the advanced-tier Ethernet server's web page pattern to view the access log remotely instead of only via the SD card.
