#include <Arduino.h>

#define MQ135_PIN 34
#define RELAY_PIN 16

// Nếu relay của bạn active LOW thì đảo hai dòng này.
#define RELAY_ON  HIGH
#define RELAY_OFF LOW

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, RELAY_ON);

    analogReadResolution(12);

    Serial.println();
    Serial.println("=================================");
    Serial.println("MQ135 ADC DUAL-RANGE DIAGNOSTIC");
    Serial.println("Relay ON - warming for 60 seconds");
    Serial.println("=================================");

    for (int i = 60; i > 0; i--) {
        Serial.printf("Warm-up: %d s\n", i);
        delay(1000);
    }

    Serial.println("Start ADC measurement");
}

void loop() {
    // Đọc ở attenuation 11 dB
    analogSetPinAttenuation(MQ135_PIN, ADC_11db);
    delay(20);

    long raw11Sum = 0;
    long mv11Sum = 0;

    for (int i = 0; i < 30; i++) {
        raw11Sum += analogRead(MQ135_PIN);
        mv11Sum += analogReadMilliVolts(MQ135_PIN);
        delay(3);
    }

    float raw11 = raw11Sum / 30.0f;
    float mv11 = mv11Sum / 30.0f;

    // Đọc ở attenuation 0 dB
    analogSetPinAttenuation(MQ135_PIN, ADC_0db);
    delay(20);

    long raw0Sum = 0;
    long mv0Sum = 0;

    for (int i = 0; i < 30; i++) {
        raw0Sum += analogRead(MQ135_PIN);
        mv0Sum += analogReadMilliVolts(MQ135_PIN);
        delay(3);
    }

    float raw0 = raw0Sum / 30.0f;
    float mv0 = mv0Sum / 30.0f;

    // Trả về 11 dB
    analogSetPinAttenuation(MQ135_PIN, ADC_11db);

    Serial.printf(
        "11dB: raw=%7.1f, mv=%7.1f | "
        "0dB: raw=%7.1f, mv=%7.1f\n",
        raw11,
        mv11,
        raw0,
        mv0
    );

    delay(1000);
}