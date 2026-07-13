#include "Cloud.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>

Cloud::Cloud() {}

void Cloud::begin() {
    // Enable station mode and connect
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.println("[Cloud] Wi-Fi initialization started.");
}

bool Cloud::connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    Serial.print("[Cloud] Wi-Fi disconnected. Reconnecting");
    
    // Attempt connection for up to 10 seconds (20 * 500ms)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        esp_task_wdt_reset(); // Feed watchdog during blocking Wi-Fi connection attempts
        delay(500);
        Serial.print(".");
        attempts++;
    }


    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[Cloud] Connected to Wi-Fi successfully!");
        Serial.print("[Cloud] IP Address: ");
        Serial.println(WiFi.localIP());
        return true;
    }

    Serial.println("\n[Cloud] Wi-Fi connection timeout!");
    return false;
}

bool Cloud::isWiFiConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

bool Cloud::publishToSupabase(const SensorRecord& data) {
    if (isnan(data.temperature) && isnan(data.humidity) && isnan(data.mq135_ppm) && isnan(data.dust_density)) {
        Serial.println("[Cloud] Publish aborted: All sensors returned NAN. Refusing to send empty payload.");
        return false;
    }

    if (!connectWiFi()) {
        Serial.println("[Cloud] Publish aborted: WiFi connection could not be established.");
        return false;
    }

    HTTPClient http;
    
    // Initialize HTTP Client with the REST endpoint URL
    if (!http.begin(SUPABASE_URL)) {
        Serial.println("[Cloud] HTTP setup failed: Invalid URL.");
        return false;
    }

    // Set REST API Headers required by Supabase
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", SUPABASE_KEY);

    // Format Authorization header: Bearer <key>
    String authHeader = "Bearer " + String(SUPABASE_KEY);
    http.addHeader("Authorization", authHeader.c_str());

    // Create JSON document (works with both ArduinoJson v6 and v7)
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    StaticJsonDocument<256> doc;
#endif

    // Populate JSON fields matching the schema requested (NAN resolves to null)
    if (isnan(data.temperature)) {
        doc["temperature"] = nullptr;
    } else {
        doc["temperature"] = data.temperature;
    }

    if (isnan(data.humidity)) {
        doc["humidity"] = nullptr;
    } else {
        doc["humidity"] = data.humidity;
    }

    if (isnan(data.mq135_ppm)) {
        doc["mq135_ppm"] = nullptr;
    } else {
        doc["mq135_ppm"] = data.mq135_ppm;
    }

    if (isnan(data.dust_density)) {
        doc["dust_density"] = nullptr;
    } else {
        doc["dust_density"] = data.dust_density;
    }

    if (isnan(data.mq135_v)) {
        doc["mq135_v"] = nullptr;
    } else {
        doc["mq135_v"] = data.mq135_v;
    }

    doc["mq135_raw"] = data.mq135_raw;
    if (isnan(data.gas_index)) doc["gas_index"] = nullptr;
    else doc["gas_index"] = data.gas_index;
    doc["dust_raw"] = data.dust_raw;
    if (isnan(data.dust_sensor_v)) doc["dust_sensor_v"] = nullptr;
    else doc["dust_sensor_v"] = data.dust_sensor_v;

    doc["status"] = (int)data.status;
    doc["warning"] = data.warning;

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    Serial.print("[Cloud] Sending payload: ");
    Serial.println(jsonPayload);

    // Send the HTTP POST request with payload
    int httpResponseCode = http.POST(jsonPayload);

    bool success = false;
    if (httpResponseCode > 0) {
        Serial.print("[Cloud] Supabase returned HTTP response code: ");
        Serial.println(httpResponseCode);

        // Supabase REST endpoint returns 201 Created on success
        if (httpResponseCode == 200 || httpResponseCode == 201) {
            Serial.println("[Cloud] Data successfully logged in database.");
            success = true;
        } else {
            String responseString = http.getString();
            Serial.print("[Cloud] Server returned error details: ");
            Serial.println(responseString);
        }
    } else {
        Serial.print("[Cloud] HTTP POST Request failed. Error code: ");
        Serial.println(http.errorToString(httpResponseCode).c_str());
    }

    // Free resources
    http.end();
    return success;
}

void Cloud::setupOTA(const char* hostname) {
    ArduinoOTA.setHostname(hostname);
    
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_SPIFFS
            type = "filesystem";
        }
        Serial.println("[OTA] Start updating " + type);
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] Update completed successfully.");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    ArduinoOTA.begin();
    Serial.println("[Cloud] OTA updates initialized.");
}
