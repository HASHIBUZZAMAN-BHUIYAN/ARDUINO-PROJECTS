# Sound-Activated Clap Switch

A sound sensor module listens for a sharp clap and toggles a relay-driven lamp on/off, like a classic "Clapper" switch.

## Difficulty & Board

**Tier:** Basic
**Board:** Arduino Nano

Reasoning: a small, single-sensor novelty gadget that's typically built into or taped near a lamp base — the Nano's compact size fits that "hidden inside the base" use case better than a full Uno.

## Components

| Part | Qty |
|---|---|
| Arduino Nano | 1 |
| Sound detection sensor module (e.g. KY-038, digital output) | 1 |
| 1-channel relay module | 1 |
| LED (stands in for a lamp) | 1 |
| Breadboard | 1 |
| Jumper wires | 6 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| 5V | Sound sensor VCC, relay VCC | |
| GND | Sound sensor GND, relay GND | shared ground |
| D2 | Sound sensor DOUT | digital pulse on loud sound |
| D4 | Relay IN | switches the "lamp" |
| Relay COM/NO | LED (+ current-limit resistor) | simulates a mains lamp safely; **do not** wire actual mains voltage for this beginner build |

## How It Works

The sensor module has an onboard comparator that outputs a brief digital pulse (`DOUT`) whenever the microphone picks up a sound louder than its trimmer-set threshold. The sketch:

1. Watches for that pulse (a rising edge on `D2`).
2. Requires **two claps within a ~1 second window** to toggle the relay — a single random noise (door slam, cough) is ignored, but two claps close together are treated as an intentional command. This is the same double-clap pattern real "Clapper" switches use to avoid false triggers.
3. Each confirmed double-clap flips the relay state (on → off, or off → on) and prints the new state to Serial.

This teaches edge detection plus a simple time-windowed pattern match — turning a noisy single sensor into a deliberate two-step "gesture."

## Setup & Flashing

1. Wire the sound sensor and relay as above. Adjust the sensor's onboard sensitivity trimmer with a small screwdriver until a normal clap reliably triggers `DOUT` (test with the Serial Monitor first).
2. Open `src/clap_switch.ino` in the Arduino IDE.
3. Select **Tools > Board > Arduino Nano** (choose the correct processor/bootloader variant for your specific Nano) and the correct COM port.
4. Upload — no external libraries required.
5. Clap twice quickly near the sensor and confirm the relay/LED toggles.

## Extensions

- Require a specific clap pattern (e.g. clap-clap-pause-clap) for a slightly more deliberate "passcode."
- Drive multiple relays/lamps, cycling through them with each double-clap.
- Add a timeout auto-off after N minutes so a lamp never gets left on by accident.
