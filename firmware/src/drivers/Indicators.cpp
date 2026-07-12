#include "Indicators.h"

Indicators::Indicators() : _muted(false) {}

void Indicators::begin() {
    // Configure warning output pins
    pinMode(WARNING_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    // Initial state is inactive (off)
    digitalWrite(WARNING_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    _muted = false;
}

void Indicators::update(bool warning) {
    if (warning) {
        digitalWrite(WARNING_LED_PIN, HIGH);
        // Only sound the buzzer if it is not currently muted
        if (!_muted) {
            digitalWrite(BUZZER_PIN, HIGH);
        } else {
            digitalWrite(BUZZER_PIN, LOW);
        }
    } else {
        digitalWrite(WARNING_LED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
        // Reset mute status when alarm clears, so next alarm sounds normally
        _muted = false;
    }
}

void Indicators::turnOff() {
    digitalWrite(WARNING_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
}

void Indicators::mute() {
    _muted = true;
    digitalWrite(BUZZER_PIN, LOW); // Turn off immediately when muted
    Serial.println("[Indicators] Buzzer muted.");
}


