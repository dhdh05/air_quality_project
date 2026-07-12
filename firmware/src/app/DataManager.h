#pragma once

#include "Config.h"
#include <ArduinoJson.h>

class DataManager {
public:
    // Constructor
    DataManager();
    
    // Initializes the SPIFFS filesystem. Returns true if successful.
    bool begin();

    // Serializes and appends a SensorRecord to the local cache file
    bool cacheRecord(const SensorRecord& record);

    // Checks if there is a cache file present on SPIFFS
    bool hasCachedRecords();

    // Reads each cached JSON record, attempts to upload it via Cloud,
    // and updates the cache file with only the failed/remaining entries.
    void processCachedRecords(class Cloud& cloud);

private:
    const char* _cachePath = "/cache.txt";
};
