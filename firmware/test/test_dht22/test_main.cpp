#include <Arduino.h>
#include <DHT.h>

#define DHTPIN 14
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
unsigned long previousMillis = 0;
const unsigned long intervalMs = 2000;

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println("Test DHT22");
  dht.begin();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= intervalMs) {
    previousMillis = currentMillis;

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Loi doc DHT22");
    } else {
      Serial.print("Nhiet do: ");
      Serial.print(temperature);
      Serial.print(" C  Do am: ");
      Serial.print(humidity);
      Serial.println(" %");
    }
  }
}
