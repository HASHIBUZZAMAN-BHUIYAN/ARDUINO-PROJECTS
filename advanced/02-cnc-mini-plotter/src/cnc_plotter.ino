/*
  Stepper Motor CNC Mini Plotter
  ----------------------------------
  Reads a tiny G-code-like command language over Serial and drives a 2-axis
  stepper plotter with pen-lift servo. Board: Arduino Mega 2560.

  Supported commands (send one per line, e.g. from Serial Monitor):
    G0 X<mm> Y<mm>   - pen-up travel move
    G1 X<mm> Y<mm>   - pen-down drawing move
    PENUP / PENDOWN  - move only the pen servo
    HOME             - drive both axes to their limit switches (if wired) and zero
*/

#include <AccelStepper.h>
#include <Servo.h>

const uint8_t X_STEP = 2, X_DIR = 3, X_ENABLE = 4;
const uint8_t Y_STEP = 5, Y_DIR = 6, Y_ENABLE = 7;
const uint8_t SERVO_PIN = 9;
const uint8_t X_LIMIT_PIN = 30;
const uint8_t Y_LIMIT_PIN = 31;

// Adjust to your hardware: (motor steps/rev * microstep setting) / (mm per revolution)
const float STEPS_PER_MM = 80.0;

const uint8_t PEN_UP_ANGLE = 30;
const uint8_t PEN_DOWN_ANGLE = 90;

AccelStepper xAxis(AccelStepper::DRIVER, X_STEP, X_DIR);
AccelStepper yAxis(AccelStepper::DRIVER, Y_STEP, Y_DIR);
Servo penServo;

String inputLine = "";

void setup() {
  Serial.begin(9600);

  pinMode(X_ENABLE, OUTPUT);
  pinMode(Y_ENABLE, OUTPUT);
  digitalWrite(X_ENABLE, LOW); // A4988 ENABLE is active-LOW: LOW = motor energized
  digitalWrite(Y_ENABLE, LOW);

  pinMode(X_LIMIT_PIN, INPUT_PULLUP);
  pinMode(Y_LIMIT_PIN, INPUT_PULLUP);

  xAxis.setMaxSpeed(2000);
  xAxis.setAcceleration(800);
  yAxis.setMaxSpeed(2000);
  yAxis.setAcceleration(800);

  penServo.attach(SERVO_PIN);
  penServo.write(PEN_UP_ANGLE);

  Serial.println("CNC plotter ready. Send G0/G1/PENUP/PENDOWN/HOME commands.");
}

void loop() {
  readSerialCommands();
  xAxis.run(); // must be called continuously (non-blocking) for smooth stepping
  yAxis.run();
}

void readSerialCommands() {
  while (Serial.available()) {
    char c = Serial.read();
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
  line.toUpperCase();
  Serial.print("> "); Serial.println(line);

  if (line.startsWith("G0") || line.startsWith("G1")) {
    bool penDown = line.startsWith("G1");
    float x, y;
    if (parseXY(line, x, y)) {
      penServo.write(penDown ? PEN_DOWN_ANGLE : PEN_UP_ANGLE);
      delay(150); // let the servo finish moving before starting the axis move
      moveToBlocking(x, y);
      Serial.println("OK");
    } else {
      Serial.println("ERROR: could not parse X/Y");
    }
  } else if (line == "PENUP") {
    penServo.write(PEN_UP_ANGLE);
  } else if (line == "PENDOWN") {
    penServo.write(PEN_DOWN_ANGLE);
  } else if (line == "HOME") {
    homeAxes();
  } else {
    Serial.println("ERROR: unknown command");
  }
}

// Extracts X and Y millimeter values from a line like "G1 X50 Y10".
bool parseXY(const String &line, float &x, float &y) {
  int xIdx = line.indexOf('X');
  int yIdx = line.indexOf('Y');
  if (xIdx == -1 || yIdx == -1) return false;

  x = line.substring(xIdx + 1, yIdx).toFloat();
  y = line.substring(yIdx + 1).toFloat();
  return true;
}

// Blocks until both axes reach the target, running both steppers together
// each iteration so diagonal moves happen simultaneously rather than one
// axis at a time.
void moveToBlocking(float xMm, float yMm) {
  xAxis.moveTo(xMm * STEPS_PER_MM);
  yAxis.moveTo(yMm * STEPS_PER_MM);

  while (xAxis.distanceToGo() != 0 || yAxis.distanceToGo() != 0) {
    xAxis.run();
    yAxis.run();
  }
}

// Drives each axis toward its limit switch at low speed until triggered,
// then treats that position as the new zero.
void homeAxes() {
  Serial.println("Homing...");

  xAxis.setSpeed(-300);
  while (digitalRead(X_LIMIT_PIN) == HIGH) xAxis.runSpeed();
  xAxis.setCurrentPosition(0);

  yAxis.setSpeed(-300);
  while (digitalRead(Y_LIMIT_PIN) == HIGH) yAxis.runSpeed();
  yAxis.setCurrentPosition(0);

  Serial.println("Homed.");
}
