#pragma once

#include <Adafruit_SSD1306.h>
#include "Config.h"

class Display {
public:
    // Constructor
    Display();

    // Initializes the OLED screen via I2C. Returns true if successful.
    bool begin();

    // Displays a splash screen on boot
    void showSplash();

    // Displays the current system status (e.g. "Idle", "Warming up...") without sensor readings
    void showStatus(const char* status);

    // Displays the full operational screen showing all sensor readings and the current state
    void showData(const SensorRecord& data, const char* status);


    // Clears the display buffer and commits it
    void clear();

    // Puts the OLED display to sleep to prevent burn-in
    void sleep();

    // Wakes the OLED display from sleep mode
    void wakeUp();


private:
    Adafruit_SSD1306 _display;
};
