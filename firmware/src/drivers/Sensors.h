#pragma once

#include <DHT.h>
#include "Config.h"

class Sensors {
public:
    // Constructor
    Sensors();

    // Configures sensor pins, sets ADC resolution, attenuation, and begins DHT
    void begin();

    // Reads all sensors and returns the encapsulated SensorRecord structure
    SensorRecord readAll();

private:
    DHT _dht;

    // Cache to retain old valid readings briefly on temporary read failure
    float _lastTemperature;
    float _lastHumidity;
    float _lastMQ135PPM;
    float _lastDustDensity;
    int _consecutiveFailures;

    // Helper to read the analog average of a pin over multiple samples
    int readADC_Average(int pin, int samples = 30);
    int readGP2Y_TrimmedAverage();
};
