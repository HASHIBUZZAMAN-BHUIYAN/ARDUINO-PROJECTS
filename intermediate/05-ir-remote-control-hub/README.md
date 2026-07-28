# IR Remote Control Hub

Repurposes a cheap 24-button IR remote to control four relay channels — a universal "home automation hub" for lamps and small appliances, with each button toggling or selecting a channel.

## Difficulty & Board

**Tier:** Intermediate
**Board:** Arduino Uno Q

Reasoning: this is a small always-on hub that plausibly lives near a media console/wall outlet, fitting the Uno Q's compact form factor, and the pin count (1 IR receiver pin + 4 relay pins) is modest. Remember the Uno Q's GPIO is **3.3V logic** — the IR receiver module used here is 3.3V-compatible, and the relay module's `IN` pins are driven from 3.3V, which is enough to switch most opto-isolated relay boards (verify your specific relay module's minimum trigger voltage).

## Components

| Part | Qty |
|---|---|
| Arduino Uno Q | 1 |
| IR receiver module (TSOP38238 or similar, 3.3-5V tolerant) | 1 |
| Generic 24-button IR remote | 1 |
| 4-channel relay module | 1 |
| LED (stands in for each controlled appliance) | 4 |
| Breadboard | 1 |
| Jumper wires | ~10 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| 3V3 | IR receiver VCC | |
| GND | IR receiver GND, relay GND | shared ground |
| D2 | IR receiver OUT | must be an interrupt-capable pin |
| D4 | Relay 1 IN | Channel A |
| D5 | Relay 2 IN | Channel B |
| D6 | Relay 3 IN | Channel C |
| D7 | Relay 4 IN | Channel D |

## How It Works

The `IRremote` library decodes the receiver's raw pulse timing into a numeric code unique to each button on the remote. The sketch's `setup()` doesn't hard-code those codes up front — instead you run a short "learn" pass (watching Serial output while pressing each button you want to use) and paste the resulting codes into the `BUTTON_*` constants.

Four of the remote's buttons are mapped one-to-one to the four relay channels as direct toggles (press button 1 → toggle channel A, and so on). Two more buttons are mapped to "all on" / "all off" convenience commands. Every incoming code is compared against the known list with a `switch` statement; unrecognized codes (any other button on the remote) are logged to Serial but otherwise ignored.

This project's core concept is decoding an asynchronous, timing-based protocol (IR) into discrete application-level commands — a step up from the basic tier's raw digital/analog sensor reads.

## Setup & Flashing

1. Wire the IR receiver and relay module as above.
2. Install the **IRremote** library via Library Manager (this project targets the v3.x+ API).
3. Open `src/ir_remote_hub.ino`, upload with `LEARNING_MODE` set to `true`, and open the Serial Monitor at 9600 baud.
4. Press each remote button you plan to use one at a time; note the printed hex code for each.
5. Fill in the `BUTTON_*` constants with your remote's actual codes, set `LEARNING_MODE` to `false`, and re-upload.
6. Select **Tools > Board > Arduino Uno Q** and the correct COM port before uploading.

## Extensions

- Add a simple on-board schedule (using `millis()` time-of-day tracking) so a channel auto-turns-off after N minutes.
- Combine with the intermediate LCD menu project so the hub can be controlled by either the remote or the on-device menu.
- Add a "learning mode" toggle button on the hub itself (rather than a code constant) so end users can re-pair a replacement remote without re-flashing.
