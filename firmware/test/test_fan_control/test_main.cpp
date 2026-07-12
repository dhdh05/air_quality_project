#include <Arduino.h>

#define FAN_CTRL_PIN 32
unsigned long previousMillis = 0;
const unsigned long intervalMs = 3000;
bool fanOn = false;

void setup() {
  Serial.begin(115200);
  delay(10);
  pinMode(FAN_CTRL_PIN, OUTPUT);
  digitalWrite(FAN_CTRL_PIN, LOW);
  Serial.println("Test Fan Control");
  Serial.println("Quat phai dieu khien qua transistor/MOSFET, khong cap truc tiep tu GPIO ESP32");
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= intervalMs) {
    previousMillis = currentMillis;
    fanOn = !fanOn;
    digitalWrite(FAN_CTRL_PIN, fanOn ? HIGH : LOW);

    if (fanOn) {
      Serial.println("FAN ON");
    } else {
      Serial.println("FAN OFF");
    }
  }
}
