#pragma once

#include "Config.h"

class Indicators {
public:
    // Constructor
    Indicators();

    // Sets warning pins as outputs and turns them off initially
    void begin();

    // Updates warning signals (Buzzer and LED) based on warning status
    void update(bool warning);

    // Forces both the warning LED and buzzer to OFF state
    void turnOff();

    // Mutes the active buzzer until the next warning event
    void mute();


private:
    bool _muted;
};

