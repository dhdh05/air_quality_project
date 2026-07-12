#pragma once

#include <Arduino.h>

// ================= PIN DEFINITIONS =================
#define DHTPIN              14  // DHT22 Data Pin
#define MQ135_PIN           34  // MQ135 Analog In (ADC1_CH6)
#define GP2Y_PIN            35  // GP2Y1010F Analog In (ADC1_CH7)
#define GP2Y_LED_PIN        25  // GP2Y1010F LED Control Pin (active LOW)
#define WARNING_LED_PIN     26  // Status indicator LED
#define BUZZER_PIN          27  // Acoustic buzzer warning output
#define RELAY_PIN           16  // Relay controlling 5V heater/fan rail
#define BUTTON_PIN          4   // Button GPIO pin for UI interrupts

// Relay Logic Configuration
#define RELAY_ON            HIGH
#define RELAY_OFF           LOW

// ================= I2C OLED CONFIGURATION =================
#define I2C_SDA             21  // SDA Pin
#define I2C_SCL             22  // SCL Pin
#define SCREEN_WIDTH        128 // OLED display width, in pixels
#define SCREEN_HEIGHT       64  // OLED display height, in pixels
#define OLED_RESET          -1  // Reset pin (shared with ESP32)
#define OLED_ADDR           0x3C// I2C address for SSD1306 OLED (often 0x3C or 0x3D)

// ================= CALIBRATION & THRESHOLDS =================
// Voltage divider factors (R1 = 12k, R2 = 22k)
// V_ESP32 = V_sensor * 22 / (12 + 22) -> V_sensor = V_ESP32 * 1.545
#define VOLTAGE_DIVIDER_FACTOR 1.545

// MQ135 Gas Sensor Calibration Constants
#define MQ135_RL               10000.0  // Load resistor in Ohms (10k)
#define MQ135_VC               5.0      // Supply voltage (5V)
#define MQ135_R0               10000.0  // Calibrated sensor resistance in clean air (e.g. 10k)
#define MQ135_CO2_A            110.47   // Scaling factor 'a' for CO2
#define MQ135_CO2_B            -2.862   // Scaling factor 'b' for CO2
#define MQ135_THRESHOLD_VOLT   2.74     // Warning threshold in Volts (equivalent to raw ADC 2200)

// GP2Y1010F Dust Sensor Calibration Constants

#define GP2Y_ZERO_DUST_VOLTAGE 0.588    // Zero-Dust Voltage calibration offset in Volts
#define GP2Y_SENSITIVITY       0.17     // Sensitivity in V per 100 ug/m3 (0.17)

// ================= SECURITY & CREDENTIALS =================
#include "secrets.h"

// ================= TIMING PARAMETERS =================
#define SAMPLING_INTERVAL   300000ULL // Delay in IDLE state between active measurements (5 minutes)
#define WARMUP_DURATION     60000ULL  // Sensor heater warm-up duration before reading (60 seconds)

// ================= DATA ENCAPSULATION & ENUMS =================
enum SensorStatus {
    SENSOR_OK,
    SENSOR_READ_ERROR,
    SENSOR_TIMEOUT,
    OFFLINE_CACHED
};

struct SensorRecord {
    float temperature;    // DHT22 temperature in Celsius
    float humidity;       // DHT22 humidity percentage
    float mq135_ppm;      // MQ135 gas concentration in PPM
    float mq135_v;        // MQ135 raw sensor voltage in Volts
    float dust_density;   // GP2Y1010F dust density in ug/m3
    SensorStatus status;  // Status flag representing sensor state
    bool warning;         // Boolean flag representing warning condition status
};

// ================= FREERTOS MUTEX DECLARATIONS =================
// Mutex protecting access to the shared SensorRecord data
extern SemaphoreHandle_t dataMutex;

// Mutex protecting access to the shared I2C bus (OLED/Sensors)
extern SemaphoreHandle_t i2cMutex;
