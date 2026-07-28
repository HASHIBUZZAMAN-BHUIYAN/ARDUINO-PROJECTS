/*
  Bluetooth Smart Lamp
  -----------------------
  Parses simple text commands received over HC-05 Bluetooth serial to drive
  an RGB LED strip's color/brightness/effects. Board: Arduino Nano.

  Commands (send as plain text lines from a Bluetooth terminal app):
    RGB,r,g,b   e.g. RGB,255,0,128
    BRIGHT,n    e.g. BRIGHT,150   (0-255)
    FADE
    STROBE
    OFF
*/

#include <SoftwareSerial.h>

const uint8_t BT_TX_PIN = 10; // Nano -> HC-05 RXD
const uint8_t BT_RX_PIN = 11; // HC-05 TXD -> Nano
SoftwareSerial bluetooth(BT_RX_PIN, BT_TX_PIN);

const uint8_t RED_PIN = 5;
const uint8_t GREEN_PIN = 6;
const uint8_t BLUE_PIN = 9;

uint8_t targetR = 0, targetG = 0, targetB = 0;
uint8_t brightness = 255;

enum Mode { STATIC, FADE, STROBE };
Mode mode = STATIC;

String inputLine = "";

void setup() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  Serial.begin(9600);      // USB, for debugging
  bluetooth.begin(9600);   // HC-05 default baud
  Serial.println("Bluetooth lamp ready.");
}

void loop() {
  readBluetoothCommands();
  runCurrentEffect();
}

void readBluetoothCommands() {
  while (bluetooth.available()) {
    char c = bluetooth.read();
    if (c == '\n' || c == '\r') {
      if (inputLine.length() > 0) {
        handleCommand(inputLine);
        inputLine = "";
      }
    } else {
      inputLine += c;
    }
  }
}

void handleCommand(String line) {
  line.trim();
  Serial.print("Command: ");
  Serial.println(line);

  if (line.startsWith("RGB,")) {
    int r, g, b;
    if (sscanf(line.c_str(), "RGB,%d,%d,%d", &r, &g, &b) == 3) {
      targetR = constrain(r, 0, 255);
      targetG = constrain(g, 0, 255);
      targetB = constrain(b, 0, 255);
      mode = STATIC;
    }
  } else if (line.startsWith("BRIGHT,")) {
    int b;
    if (sscanf(line.c_str(), "BRIGHT,%d", &b) == 1) {
      brightness = constrain(b, 0, 255);
    }
  } else if (line == "FADE") {
    mode = FADE;
  } else if (line == "STROBE") {
    mode = STROBE;
  } else if (line == "OFF") {
    mode = STATIC;
    targetR = targetG = targetB = 0;
  } else {
    Serial.println("Unknown command.");
  }
}

void runCurrentEffect() {
  switch (mode) {
    case STATIC:
      writeColor(targetR, targetG, targetB);
      break;

    case FADE:
      fadeEffect();
      break;

    case STROBE:
      strobeEffect();
      break;
  }
}

// Scales an RGB triple by the global brightness setting before writing PWM.
void writeColor(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(RED_PIN, (uint16_t)r * brightness / 255);
  analogWrite(GREEN_PIN, (uint16_t)g * brightness / 255);
  analogWrite(BLUE_PIN, (uint16_t)b * brightness / 255);
}

void fadeEffect() {
  static uint16_t hue = 0;
  hue = (hue + 2) % 360;
  uint8_t r, g, b;
  hueToRgb(hue, r, g, b);
  writeColor(r, g, b);
  delay(20);
}

void strobeEffect() {
  static bool on = false;
  static unsigned long lastToggle = 0;
  if (millis() - lastToggle > 80) {
    on = !on;
    lastToggle = millis();
    writeColor(on ? 255 : 0, on ? 255 : 0, on ? 255 : 0);
  }
}

// Simple HSV(hue,1,1) -> RGB conversion for the fade effect's color cycling.
void hueToRgb(uint16_t hue, uint8_t &r, uint8_t &g, uint8_t &b) {
  uint8_t region = hue / 60;
  uint8_t remainder = (hue % 60) * 255 / 60;

  switch (region) {
    case 0: r = 255; g = remainder; b = 0; break;
    case 1: r = 255 - remainder; g = 255; b = 0; break;
    case 2: r = 0; g = 255; b = remainder; break;
    case 3: r = 0; g = 255 - remainder; b = 255; break;
    case 4: r = remainder; g = 0; b = 255; break;
    default: r = 255; g = 0; b = 255 - remainder; break;
  }
}
