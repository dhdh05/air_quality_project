#include <Arduino.h>

#define DUST_ADC_PIN 35
#define DUST_LED_CTRL_PIN 25
#define VOLTAGE_DIVIDER_FACTOR 1.545

void setup() {
  Serial.begin(115200);
  delay(10);
  pinMode(DUST_LED_CTRL_PIN, OUTPUT);
  digitalWrite(DUST_LED_CTRL_PIN, HIGH);
  Serial.println("Test Dust Sensor");
}

void loop() {
  digitalWrite(DUST_LED_CTRL_PIN, LOW);
  delayMicroseconds(280);

  int raw = analogRead(DUST_ADC_PIN);
  delayMicroseconds(40);

  digitalWrite(DUST_LED_CTRL_PIN, HIGH);
  delayMicroseconds(9680);

  float adcVoltage = raw * 3.3 / 4095.0;
  float sensorVoltage = adcVoltage * VOLTAGE_DIVIDER_FACTOR;
  float dustDensity = (0.17 * sensorVoltage - 0.1) * 1000.0;
  if (dustDensity < 0) {
    dustDensity = 0;
  }

  Serial.print("Raw: ");
  Serial.print(raw);
  Serial.print("  ADC V: ");
  Serial.print(adcVoltage, 3);
  Serial.print(" V  Sensor V: ");
  Serial.print(sensorVoltage, 3);
  Serial.print(" V  Dust: ");
  Serial.print(dustDensity, 2);
  Serial.println(" ug/m3");

  delay(1000);
}
