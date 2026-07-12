#include "Sensors.h"
#include <math.h>

Sensors::Sensors() 
    : _dht(DHTPIN, DHT22), 
      _lastTemperature(NAN), 
      _lastHumidity(NAN), 
      _lastMQ135PPM(NAN), 
      _lastDustDensity(NAN), 
      _consecutiveFailures(0) {}

void Sensors::begin() {
    // Configure GP2Y dust sensor LED control pin
    pinMode(GP2Y_LED_PIN, OUTPUT);
    digitalWrite(GP2Y_LED_PIN, HIGH);  // Turn OFF internal LED initially (active LOW)

    // Set ADC parameters for ESP32
    analogReadResolution(12); // 12-bit resolution (0 - 4095)
    analogSetPinAttenuation(MQ135_PIN, ADC_11db); // Attenuation for 0 - 3.3V range
    analogSetPinAttenuation(GP2Y_PIN, ADC_11db);  // Attenuation for 0 - 3.3V range

    // Initialize DHT sensor
    _dht.begin();
}

SensorRecord Sensors::readAll() {
    SensorRecord record;
    
    // 1. Read DHT22 Temperature & Humidity
    float temp = _dht.readTemperature();
    float hum = _dht.readHumidity();

    if (isnan(temp) || isinf(temp)) {
        record.temperature = NAN;
    } else {
        record.temperature = temp;
        _lastTemperature = temp;
    }

    if (isnan(hum) || isinf(hum)) {
        record.humidity = NAN;
    } else {
        record.humidity = hum;
        _lastHumidity = hum;
    }

    // 2. Read MQ135 (analog average reading)
    int rawMQ = readADC_Average(MQ135_PIN, 30);
    float mqAdcVoltage = rawMQ * 3.3 / 4095.0;
    float mq135_v = mqAdcVoltage * VOLTAGE_DIVIDER_FACTOR;
    
    Serial.printf("[Sensors] MQ135 Raw ADC: %d, Voltage: %.3f V\n", rawMQ, mq135_v);

    record.mq135_v = mq135_v; // Save the raw voltage for display

    if (rawMQ <= 2) { // Only trigger NAN if the pin is completely grounded/disconnected
        record.mq135_ppm = NAN;
    } else {
        // Calculate MQ135 sensor resistance (Rs) and calculate PPM using Rs/R0 curve
        if (mq135_v <= 0.0) mq135_v = 0.001; 
        if (mq135_v >= MQ135_VC) mq135_v = MQ135_VC - 0.001; 
        float Rs = MQ135_RL * (MQ135_VC - mq135_v) / mq135_v;
        float ratio = Rs / MQ135_R0;
        float mq135_ppm = MQ135_CO2_A * pow(ratio, MQ135_CO2_B);

        if (isnan(mq135_ppm) || isinf(mq135_ppm)) {
            record.mq135_ppm = NAN;
        } else {
            record.mq135_ppm = mq135_ppm;
            _lastMQ135PPM = mq135_ppm;
        }
    }

    // 3. Read GP2Y Dust Sensor
    // GP2Y LED is active LOW. Turn it ON
    digitalWrite(GP2Y_LED_PIN, LOW);
    delayMicroseconds(280); // Stabilization window before ADC read

    int rawDust = analogRead(GP2Y_PIN);

    delayMicroseconds(40);
    digitalWrite(GP2Y_LED_PIN, HIGH); // Turn off internal LED
    delayMicroseconds(9680); // Rest of the 10ms cycle period

    float dustAdcVoltage = rawDust * 3.3 / 4095.0;
    float dustSensorVoltage = dustAdcVoltage * VOLTAGE_DIVIDER_FACTOR;

    // Use original formula from old version code: (0.17 * sensorVoltage - 0.1) * 1000.0
    float dust = (0.17 * dustSensorVoltage - 0.1) * 1000.0;
    if (dust < 0.0) {
        dust = 0.0;
    }

    Serial.printf("[Sensors] Dust Raw ADC: %d, Voltage: %.3f V, Density: %.1f ug/m3\n", rawDust, dustSensorVoltage, dust);

    if (isnan(dust) || isinf(dust)) {
        record.dust_density = NAN;
    } else {
        record.dust_density = dust;
        _lastDustDensity = dust;
    }

    // Status: if ALL sensors are failed, set SENSOR_READ_ERROR. Otherwise SENSOR_OK (partial data ok)
    if (isnan(record.temperature) && isnan(record.humidity) && isnan(record.mq135_ppm) && isnan(record.dust_density)) {
        record.status = SENSOR_READ_ERROR;
    } else {
        record.status = SENSOR_OK;
    }




    // Determine warning threshold based on MQ135 voltage level (warning when > MQ135_THRESHOLD_VOLT)
    record.warning = (record.status != SENSOR_READ_ERROR && !isnan(record.mq135_v) && record.mq135_v > MQ135_THRESHOLD_VOLT);

    return record;
}

int Sensors::readADC_Average(int pin, int samples) {
    long sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += analogRead(pin);
        delay(2); // Short delay between analog readings for stability
    }
    return sum / samples;
}

