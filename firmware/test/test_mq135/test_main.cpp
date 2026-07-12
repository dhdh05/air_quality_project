#include <Arduino.h>

#define MQ135_ADC_PIN 34
const unsigned long intervalMs = 1000;
unsigned long previousMillis = 0;

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println("Test MQ135 ADC");
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= intervalMs) {
    previousMillis = currentMillis;

    long sum = 0;
    const int sampleCount = 20;
    for (int i = 0; i < sampleCount; i++) {
      sum += analogRead(MQ135_ADC_PIN);
      delay(5);
    }

    float rawAvg = sum / (float)sampleCount;
    float voltage = rawAvg * 3.3 / 4095.0;

    Serial.print("Raw trung binh: ");
    Serial.print(rawAvg, 1);
    Serial.print("  ADC Voltage: ");
    Serial.print(voltage, 3);
    Serial.println(" V");
  }
}
