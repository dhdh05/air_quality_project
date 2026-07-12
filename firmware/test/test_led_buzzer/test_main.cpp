#include <Arduino.h>

#define WARNING_LED_PIN 26
#define BUZZER_PIN 27

unsigned long previousMillis = 0;
const unsigned long intervalMs = 1000;
bool stateOn = false;

void setup() {
  Serial.begin(115200);
  delay(10);
  pinMode(WARNING_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(WARNING_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("Test LED va Buzzer");
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= intervalMs) {
    previousMillis = currentMillis;
    stateOn = !stateOn;

    digitalWrite(WARNING_LED_PIN, stateOn ? HIGH : LOW);
    digitalWrite(BUZZER_PIN, stateOn ? HIGH : LOW);

    if (stateOn) {
      Serial.println("LED ON  BUZZER ON");
    } else {
      Serial.println("LED OFF BUZZER OFF");
    }
  }
}
