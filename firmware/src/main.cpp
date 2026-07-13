#include <Arduino.h>
#include <Wire.h>
#include <esp_task_wdt.h>
#include <ArduinoOTA.h>
#include "Config.h"
#include "drivers/Display.h"
#include "drivers/Sensors.h"
#include "drivers/Indicators.h"
#include "app/Cloud.h"
#include "app/DataManager.h"

// ================= STATE MACHINE STATES =================
enum SystemState {
    STATE_IDLE,             // Relay heater/fan OFF, wait for next sample interval
    STATE_WARMUP,           // Relay heater/fan ON, wait for sensor stabilization
    STATE_READ_AND_PUBLISH  // Read sensors, update OLED, POST to Supabase, Relay OFF
};

// ================= GLOBAL STATE MACHINE VARIABLES =================
volatile SystemState currentState = STATE_WARMUP;

SensorRecord sharedRecord         = {0};
char globalStatus[48]             = "Dang khoi dong...";
bool hasValidReadings             = false;

// ================= GLOBAL MODULE INSTANCES =================
Display     display;
Sensors     sensors;
Indicators  indicators;
Cloud       cloud;
DataManager dataManager;

// ================= FREERTOS MUTEX DEFINITIONS =================
SemaphoreHandle_t dataMutex = NULL;
SemaphoreHandle_t i2cMutex  = NULL;

// ================= FREERTOS TASK HANDLES =================
TaskHandle_t xTaskSensorsHandle = NULL;
TaskHandle_t xTaskNetworkHandle = NULL;
TaskHandle_t xTaskOLEDHandle    = NULL;

// ================= FREERTOS TASK DEFINITIONS =================

// TaskSensors: Runs on Core 1. Governs state machine and reads sensors.
void TaskSensorsCode(void* pvParameters) {
    Serial.println("[TaskSensors] Started on Core 1.");
    
    while (true) {
        // --- 1. STATE: WARMUP ---
        currentState = STATE_WARMUP;
        Serial.println("[TaskSensors] Relay ON. Warming up sensors...");
        digitalWrite(RELAY_PIN, RELAY_ON);

        const int warmupSeconds = WARMUP_DURATION / 1000;
        TickType_t nextSecond = xTaskGetTickCount();
        for (int remaining = warmupSeconds; remaining > 0; remaining--) {
            if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
                snprintf(globalStatus, sizeof(globalStatus), "Lam nong: %ds...", remaining);
                xSemaphoreGive(dataMutex);
            }
            Serial.printf("[TaskSensors] Warm-up: %d s remaining.\n", remaining);
            vTaskDelayUntil(&nextSecond, pdMS_TO_TICKS(1000));
        }

        // --- 2. STATE: READ ---
        currentState = STATE_READ_AND_PUBLISH;
        Serial.println("[TaskSensors] Reading sensors...");
        
        if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
            strncpy(globalStatus, "Dang do...", sizeof(globalStatus));
            xSemaphoreGive(dataMutex);
        }

        // Collect sensor measurements (safe from concurrent OLED drawing)
        SensorRecord record = sensors.readAll();

        // Update the globally shared SensorRecord
        if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
            sharedRecord.temperature = record.temperature;
            sharedRecord.humidity = record.humidity;
            sharedRecord.mq135_raw = record.mq135_raw;
            sharedRecord.mq135_ppm = record.mq135_ppm;
            sharedRecord.mq135_v = record.mq135_v;
            sharedRecord.gas_index = record.gas_index;
            sharedRecord.dust_raw = record.dust_raw;
            sharedRecord.dust_sensor_v = record.dust_sensor_v;
            sharedRecord.dust_density = record.dust_density;
            sharedRecord.status = record.status;
            sharedRecord.warning = record.warning;
            
            if (record.status == SENSOR_OK) {
                hasValidReadings = true;
            }

            strncpy(globalStatus, "Dang gui...", sizeof(globalStatus));
            xSemaphoreGive(dataMutex);
        }

        // Trigger warning indicators (LED / Buzzer)
        indicators.update(record.warning);

        // Notify TaskNetwork that new data is ready for upload
        if (xTaskNetworkHandle != NULL) {
            xTaskNotifyGive(xTaskNetworkHandle);
        }

        // --- 3. STATE: IDLE ---
        digitalWrite(RELAY_PIN, RELAY_OFF);
        currentState = STATE_IDLE;
        Serial.println("[TaskSensors] Relay OFF. Entering IDLE mode.");

        // Block until the normal interval expires or TaskButton requests a manual cycle.
        uint32_t notified = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(SAMPLING_INTERVAL));
        if (notified > 0) {
            Serial.println("[TaskSensors] Manual measurement request accepted.");
        } else {
            Serial.println("[TaskSensors] Sampling interval elapsed. Waking up.");
        }
    }
}

// TaskOLED: Runs on Core 1. Updates OLED display dynamically.
void TaskOLEDCode(void* pvParameters) {
    Serial.println("[TaskOLED] Started on Core 1.");
    
    while (true) {
        SensorRecord localRecord;
        char statusCopy[48];

        // Retrieve local copy of shared metrics safely via Mutex
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            localRecord = sharedRecord;
            strncpy(statusCopy, globalStatus, sizeof(statusCopy));
            xSemaphoreGive(dataMutex);
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        // Update the display utilizing i2cMutex to prevent bus conflict
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            if (currentState == STATE_IDLE) {
                if (hasValidReadings) {
                    display.showData(localRecord, "Idle");
                } else {
                    display.showStatus("Idle");
                }
            } else if (currentState == STATE_WARMUP) {
                // Always show the live countdown. Using a fixed "Lam nong..."
                // label after the first valid record made later cycles look stuck.
                display.showStatus(statusCopy);
            } else {
                display.showStatus(statusCopy);
            }
            xSemaphoreGive(i2cMutex);
        }

        // Update screen 4 times per second (250ms)
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

// TaskNetwork: Runs on Core 0. Handles Wi-Fi, OTA, and Supabase database publication.
void TaskNetworkCode(void* pvParameters) {
    Serial.println("[TaskNetwork] Started on Core 0.");
    
    while (true) {
        // Handle non-blocking Wi-Fi auto-reconnection and OTA callbacks
        cloud.connectWiFi();
        ArduinoOTA.handle();

        // If online, check and upload queued records from SPIFFS cache
        if (cloud.isWiFiConnected() && dataManager.hasCachedRecords()) {
            dataManager.processCachedRecords(cloud);
        }

        // Wait for notification that new data is ready for upload (1s timeout)
        uint32_t notified = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));
        if (notified > 0) {
            SensorRecord localRecord;
            if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
                localRecord = sharedRecord;
                xSemaphoreGive(dataMutex);
            }

            bool publishSuccess = false;
            if (cloud.isWiFiConnected()) {
                publishSuccess = cloud.publishToSupabase(localRecord);
            }

            if (publishSuccess) {
                Serial.println("[TaskNetwork] Live record posted to Supabase successfully.");
                if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
                    strncpy(globalStatus, "Gui thanh cong!", sizeof(globalStatus));
                    xSemaphoreGive(dataMutex);
                }
            } else {
                Serial.println("[TaskNetwork] Network offline. Appending record to SPIFFS cache...");
                dataManager.cacheRecord(localRecord);
                if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
                    strncpy(globalStatus, "Da luu cache!", sizeof(globalStatus));
                    xSemaphoreGive(dataMutex);
                }
            }

            // Give the user 1.5s to see the status before screen updates
            vTaskDelay(pdMS_TO_TICKS(1500));
        }
    }
}

// ================= SYSTEM SETUP =================
void setup() {
    // Initialize Serial Port
    Serial.begin(115200);
    delay(500);
    Serial.println("\n==============================================");
    Serial.println("  ESP32 AIR QUALITY MONITOR (FreeRTOS) BOOTING...");
    Serial.println("==============================================");

    // Initialize Relay pin
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, RELAY_ON); // Turn Relay ON on boot
    Serial.println("[System] Relay power rail initialized (ON).");

    // Initialize button pin
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    Serial.printf("[System] Button GPIO %d: short=mute, hold 2s=manual measurement.\n", BUTTON_PIN);

    // Initialize I2C Bus
    Wire.begin(I2C_SDA, I2C_SCL);
    Serial.println("[System] I2C Bus initialized.");

    // Initialize display driver
    if (!display.begin()) {
        Serial.println("[System] ERROR: OLED SSD1306 initialization failed!");
        while (true) {
            delay(100);
        }
    }
    Serial.println("[System] OLED display initialized.");
    display.showSplash();
    delay(2000);

    // Initialize drivers
    indicators.begin();
    sensors.begin();
    cloud.begin();
    cloud.setupOTA("AirQualityMonitor");

    // Initialize SPIFFS data caching manager
    if (!dataManager.begin()) {
        Serial.println("[System] ERROR: DataManager SPIFFS setup failed!");
    }

    // Create Mutexes for resource protection
    dataMutex = xSemaphoreCreateMutex();
    i2cMutex  = xSemaphoreCreateMutex();

    if (dataMutex == NULL || i2cMutex == NULL) {
        Serial.println("[System] ERROR: FreeRTOS Mutex creation failed! Halting.");
        while (true) {
            delay(100);
        }
    }
    Serial.println("[System] Mutexes created successfully.");

    // Create FreeRTOS Tasks
    xTaskCreatePinnedToCore(
        TaskSensorsCode,
        "TaskSensors",
        4096,
        NULL,
        2,                  // Priority 2
        &xTaskSensorsHandle,
        1                   // Pinned to Core 1
    );

    xTaskCreatePinnedToCore(
        TaskOLEDCode,
        "TaskOLED",
        4096,
        NULL,
        1,                  // Priority 1
        &xTaskOLEDHandle,
        1                   // Pinned to Core 1
    );

    xTaskCreatePinnedToCore(
        TaskNetworkCode,
        "TaskNetwork",
        8192,
        NULL,
        1,                  // Priority 1
        &xTaskNetworkHandle,
        0                   // Pinned to Core 0 (Protocol Core)
    );

    // Initialize Hardware Watchdog Timer (WDT) to 15 seconds
    esp_task_wdt_init(15, true); // 15s timeout, panic reset enabled
    esp_task_wdt_add(NULL);      // Add main Arduino loop task to WDT
    Serial.println("[System] Hardware Watchdog Timer (WDT) initialized (15s).");
}

// Poll the UI button from Arduino's loop task so it cannot compete with
// the timing-critical sensor state machine at priority 2.
void processButton() {
    static bool stablePressed = false;
    static bool lastRawPressed = false;
    static bool longPressHandled = false;
    static uint32_t rawChangedAt = millis();
    static uint32_t pressedAt = 0;
    static uint32_t lastManualRequestAt = 0;

    const uint32_t now = millis();
    const bool rawPressed = digitalRead(BUTTON_PIN) == LOW;

    if (rawPressed != lastRawPressed) {
        lastRawPressed = rawPressed;
        rawChangedAt = now;
    }

    if (rawPressed != stablePressed && now - rawChangedAt >= BUTTON_DEBOUNCE_MS) {
        stablePressed = rawPressed;
        if (stablePressed) {
            pressedAt = now;
            longPressHandled = false;
        } else if (!longPressHandled) {
            indicators.mute();
            Serial.println("[Button] Short press: buzzer muted.");
        }
    }

    if (stablePressed && !longPressHandled && now - pressedAt >= BUTTON_LONG_PRESS_MS) {
        longPressHandled = true;
        const bool cooldownPassed = lastManualRequestAt == 0 ||
            now - lastManualRequestAt >= MANUAL_MEASURE_COOLDOWN_MS;

        if (currentState != STATE_IDLE) {
            Serial.println("[Button] Long press ignored: measurement already in progress.");
        } else if (!cooldownPassed) {
            Serial.println("[Button] Long press ignored: cooldown active.");
        } else if (xTaskSensorsHandle != NULL) {
            lastManualRequestAt = now;
            xTaskNotifyGive(xTaskSensorsHandle);
            Serial.println("[Button] Long press: manual measurement requested.");
        }
    }
}

// Main loop: handles button polling and feeds the watchdog.
void loop() {
    processButton();
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(20));
}
