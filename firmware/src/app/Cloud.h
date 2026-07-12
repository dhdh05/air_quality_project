#pragma once

#include "Config.h"

class Cloud {
public:
    // Constructor
    Cloud();

    // Sets up WiFi mode and starts connecting asynchronously
    void begin();

    // Ensures WiFi connection is established. Returns true if connected.
    bool connectWiFi();

    // Checks current connection status
    bool isWiFiConnected();

    // Compiles a JSON payload and performs an HTTP POST to the Supabase endpoint
    bool publishToSupabase(const SensorRecord& data);


    // Initializes OTA (Over-the-Air) programming settings
    void setupOTA(const char* hostname = "AirQualityMonitor");
};

