/*
  Soil Moisture Plant Alert
  ---------------------------
  Averages a capacitive soil sensor reading and alerts (LED + one chirp)
  when the soil dries out. Board: Arduino Uno Q (3.3V logic).
*/

const uint8_t SENSOR_PIN = A0;
const uint8_t LED_PIN = 5;
const uint8_t BUZZER_PIN = 6;

// Calibrate these against your own sensor: dip in water vs. dry air/soil.
// Capacitive sensors read HIGHER when drier.
const int DRY_THRESHOLD = 600;
const uint8_t SAMPLES = 10;

bool alerting = false;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  int level = readAveraged();
  Serial.print("moisture (raw, higher=drier): ");
  Serial.println(level);

  bool isDry = level > DRY_THRESHOLD;

  if (isDry && !alerting) {
    alerting = true;
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 1500, 200); // one short chirp, non-blocking
  } else if (!isDry && alerting) {
    alerting = false;
    digitalWrite(LED_PIN, LOW);
  }

  delay(2000); // soil moisture changes slowly; no need to poll fast
}

// Soil readings are noisy; averaging several samples smooths that out.
int readAveraged() {
  long sum = 0;
  for (uint8_t i = 0; i < SAMPLES; i++) {
    sum += analogRead(SENSOR_PIN);
    delay(10);
  }
  return sum / SAMPLES;
}
