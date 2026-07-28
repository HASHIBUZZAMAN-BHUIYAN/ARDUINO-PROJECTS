# LCD Menu System

A rotary encoder scrolls through a multi-level menu on a 16x2 LCD to control three relay-driven outputs and adjust an LCD-backlight brightness setting that's saved to EEPROM.

## Difficulty & Board

**Tier:** Intermediate
**Board:** Arduino Uno

Reasoning: an LCD (6 pins) + rotary encoder (3 pins) + 3 relay channels is a moderate, very standard pin count — squarely an Uno-sized project with no motors or high-bandwidth peripherals involved.

## Components

| Part | Qty |
|---|---|
| Arduino Uno | 1 |
| 16x2 character LCD (HD44780, parallel) | 1 |
| Rotary encoder module (with push-button) | 1 |
| 10 kΩ potentiometer (LCD contrast) | 1 |
| 3-channel relay module | 1 |
| LED (stands in for a controlled device) | 3 |
| Breadboard | 1 |
| Jumper wires | ~16 |

## Wiring

| Arduino Pin | Component | Notes |
|---|---|---|
| D7 | LCD RS | |
| D8 | LCD EN | |
| D9 | LCD D4 | |
| D10 | LCD D5 | |
| D11 | LCD D6 | |
| D12 | LCD D7 | |
| D6 (PWM) | LCD backlight (via transistor or directly if module allows) | brightness adjustable from the menu |
| A0 | Encoder CLK | |
| A1 | Encoder DT | |
| A2 | Encoder SW (push-button) | |
| D2 | Relay 1 IN | channel "Device 1" |
| D3 | Relay 2 IN | channel "Device 2" |
| D4 | Relay 3 IN | channel "Device 3" |

## How It Works

The menu is modeled as a simple state machine with a small array of menu item labels and a `menuIndex`. Turning the encoder moves `menuIndex` up/down (decoded by comparing the current `CLK`/`DT` pin states against their previous states — the classic quadrature-decoding trick for cheap rotary encoders). Pressing the encoder's button "selects" the highlighted item:

- **Toggle Device 1/2/3** — flips the corresponding relay and redraws an ON/OFF indicator next to that menu line.
- **Backlight brightness** — enters a sub-mode where further encoder turns adjust a 0-255 PWM value on the backlight pin instead of scrolling the menu, until pressed again to confirm and return to the main menu. The chosen value is written to `EEPROM` so it survives a power cycle.

This project's core concept is state-machine UI design: the same physical control (one encoder + one button) does different things depending on which menu "mode" is currently active, which is how essentially every embedded device menu (microwaves, thermostats, 3D printers) is built.

## Setup & Flashing

1. Wire the LCD, encoder, and relay module as above; set contrast with the pot.
2. No non-built-in libraries are required — this project uses `LiquidCrystal` and `EEPROM`, both bundled with the Arduino IDE.
3. Open `src/lcd_menu.ino` in the Arduino IDE.
4. Select **Tools > Board > Arduino Uno** and the correct COM port.
5. Upload, then turn the encoder to scroll the menu, press to select/toggle, and confirm the backlight brightness setting persists after a power cycle.

## Extensions

- Add a fourth menu item that shows live sensor readings (temperature, etc.) instead of a toggle.
- Replace the flat menu with nested submenus (e.g. "Settings > Backlight", "Settings > Contrast").
- Add a splash/idle screen that returns automatically after a period of no input.
