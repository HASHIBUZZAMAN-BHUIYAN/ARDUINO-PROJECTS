/*
  LCD Menu System
  -----------------
  Rotary-encoder-driven menu on a 16x2 LCD controlling 3 relays and a
  backlight brightness setting persisted to EEPROM. Board: Arduino Uno.
*/

#include <LiquidCrystal.h>
#include <EEPROM.h>

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
const uint8_t BACKLIGHT_PIN = 6;

const uint8_t ENC_CLK = A0;
const uint8_t ENC_DT = A1;
const uint8_t ENC_SW = A2;

const uint8_t RELAY_PINS[3] = {2, 3, 4};
bool relayState[3] = {false, false, false};

const char *MENU_LABELS[4] = {"Device 1", "Device 2", "Device 3", "Backlight"};
const uint8_t MENU_LEN = 4;
const uint8_t EEPROM_BRIGHTNESS_ADDR = 0;

int menuIndex = 0;
bool inBrightnessMode = false;
uint8_t brightness = 200;

int lastClkState;
bool lastButtonState = HIGH;

void setup() {
  lcd.begin(16, 2);
  pinMode(BACKLIGHT_PIN, OUTPUT);
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);

  for (uint8_t i = 0; i < 3; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW);
  }

  // Restore last saved backlight brightness (0-255) so it survives power cycles.
  brightness = EEPROM.read(EEPROM_BRIGHTNESS_ADDR);
  analogWrite(BACKLIGHT_PIN, brightness);

  lastClkState = digitalRead(ENC_CLK);
  drawMenu();
}

void loop() {
  handleEncoderRotation();
  handleButtonPress();
}

// Classic quadrature decode: a full detent registers a state change on CLK.
// Comparing CLK against DT tells us which direction the knob rotated.
void handleEncoderRotation() {
  int clkState = digitalRead(ENC_CLK);
  if (clkState != lastClkState && clkState == LOW) {
    int direction = (digitalRead(ENC_DT) != clkState) ? 1 : -1;

    if (inBrightnessMode) {
      brightness = constrain((int)brightness + direction * 15, 0, 255);
      analogWrite(BACKLIGHT_PIN, brightness);
    } else {
      menuIndex = (menuIndex + direction + MENU_LEN) % MENU_LEN;
    }
    drawMenu();
  }
  lastClkState = clkState;
}

void handleButtonPress() {
  bool buttonState = digitalRead(ENC_SW);
  if (buttonState == LOW && lastButtonState == HIGH) {
    delay(30); // debounce
    selectCurrentItem();
    drawMenu();
  }
  lastButtonState = buttonState;
}

void selectCurrentItem() {
  if (inBrightnessMode) {
    // Confirm and persist the chosen brightness.
    EEPROM.update(EEPROM_BRIGHTNESS_ADDR, brightness);
    inBrightnessMode = false;
    return;
  }

  if (menuIndex == 3) {
    inBrightnessMode = true; // enter brightness sub-mode
  } else {
    relayState[menuIndex] = !relayState[menuIndex];
    digitalWrite(RELAY_PINS[menuIndex], relayState[menuIndex] ? HIGH : LOW);
  }
}

void drawMenu() {
  lcd.clear();

  if (inBrightnessMode) {
    lcd.setCursor(0, 0);
    lcd.print("Backlight:");
    lcd.setCursor(0, 1);
    lcd.print(brightness);
    lcd.print(" (press=ok)");
    return;
  }

  lcd.setCursor(0, 0);
  lcd.print(">");
  lcd.print(MENU_LABELS[menuIndex]);

  if (menuIndex < 3) {
    lcd.setCursor(0, 1);
    lcd.print(relayState[menuIndex] ? "State: ON" : "State: OFF");
  }
}
