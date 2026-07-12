#include "DataManager.h"
#include "app/Cloud.h"
#include <SPIFFS.h>

DataManager::DataManager() {}

bool DataManager::begin() {
    // Mount SPIFFS with auto-format enabled if mounting fails initially
    if (!SPIFFS.begin(true)) {
        Serial.println("[DataManager] ERROR: SPIFFS mount failed!");
        return false;
    }
    Serial.println("[DataManager] SPIFFS filesystem mounted successfully.");
    return true;
}

bool DataManager::cacheRecord(const SensorRecord& record) {
    File file = SPIFFS.open(_cachePath, FILE_APPEND);
    if (!file) {
        Serial.println("[DataManager] ERROR: Failed to open cache file for appending!");
        return false;
    }

#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    StaticJsonDocument<256> doc;
#endif

    // Parse floats safely using isnan() checks
    if (isnan(record.temperature)) {
        doc["temperature"] = nullptr;
    } else {
        doc["temperature"] = record.temperature;
    }

    if (isnan(record.humidity)) {
        doc["humidity"] = nullptr;
    } else {
        doc["humidity"] = record.humidity;
    }

    if (isnan(record.mq135_ppm)) {
        doc["mq135_ppm"] = nullptr;
    } else {
        doc["mq135_ppm"] = record.mq135_ppm;
    }

    if (isnan(record.mq135_v)) {
        doc["mq135_v"] = nullptr;
    } else {
        doc["mq135_v"] = record.mq135_v;
    }

    if (isnan(record.dust_density)) {
        doc["dust_density"] = nullptr;
    } else {
        doc["dust_density"] = record.dust_density;
    }

    doc["status"]      = (int)record.status;

    String jsonString;
    serializeJson(doc, jsonString);
    
    file.println(jsonString);
    file.close();
    
    Serial.printf("[DataManager] Record cached offline: %s\n", jsonString.c_str());
    return true;
}

bool DataManager::hasCachedRecords() {
    return SPIFFS.exists(_cachePath);
}

void DataManager::processCachedRecords(Cloud& cloud) {
    if (!hasCachedRecords()) {
        return;
    }

    File file = SPIFFS.open(_cachePath, FILE_READ);
    if (!file) {
        Serial.println("[DataManager] ERROR: Failed to open cache file for reading!");
        return;
    }

    Serial.println("[DataManager] Found cached records. Uploading...");

    // Create a temporary cache file to store records that fail to upload
    const char* tempPath = "/temp_cache.txt";
    File tempFile = SPIFFS.open(tempPath, FILE_WRITE);
    bool anyFailure = false;

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;

#if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument doc;
#else
        StaticJsonDocument<256> doc;
#endif
        DeserializationError error = deserializeJson(doc, line);
        if (error) {
            Serial.printf("[DataManager] JSON parsing error: %s\n", error.c_str());
            continue;
        }

        // Reconstruct SensorRecord
        SensorRecord record;
        record.temperature = doc["temperature"].isNull() ? NAN : doc["temperature"].as<float>();
        record.humidity    = doc["humidity"].isNull() ? NAN : doc["humidity"].as<float>();
        record.mq135_ppm   = doc["mq135_ppm"].isNull() ? NAN : doc["mq135_ppm"].as<float>();
        record.mq135_v     = doc["mq135_v"].isNull() ? NAN : doc["mq135_v"].as<float>();
        record.dust_density = doc["dust_density"].isNull() ? NAN : doc["dust_density"].as<float>();
        record.status      = OFFLINE_CACHED; // Override status to signify it was cached offline

        // Try to upload record
        if (cloud.publishToSupabase(record)) {
            Serial.println("[DataManager] Cached record successfully posted to Supabase.");
        } else {
            Serial.println("[DataManager] Failed to post cached record. Keeping in cache.");
            if (tempFile) {
                tempFile.println(line); // Keep in cache by writing to temp file
            }
            anyFailure = true;
        }
    }

    file.close();
    if (tempFile) {
        tempFile.close();
    }

    // Replace the original cache file with the remaining unsent entries in the temp file
    SPIFFS.remove(_cachePath);
    if (anyFailure) {
        SPIFFS.rename(tempPath, _cachePath);
        Serial.println("[DataManager] Cached records processing completed. Some records remain offline.");
    } else {
        SPIFFS.remove(tempPath);
        Serial.println("[DataManager] All cached records successfully processed and cleared.");
    }
}
