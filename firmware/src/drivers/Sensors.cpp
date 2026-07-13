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
    analogSetPinAttenuation(MQ135_PIN, ADC_11db);
    delay(10);

    long mqMillivoltSum = 0;

    for (int i = 0; i < 30; i++) {
        mqMillivoltSum += analogReadMilliVolts(MQ135_PIN);
        delay(3);
    }

    float mqAdcMillivolts = mqMillivoltSum / 30.0f;
    float mqAdcVoltage = mqAdcMillivolts / 1000.0f;

    // Quy đổi ngược qua cầu chia áp
    float mq135_v =
        mqAdcVoltage * VOLTAGE_DIVIDER_FACTOR;

    // Raw tương đương để tiếp tục ghi log
    int rawMQ =
        round(mqAdcVoltage * 4095.0f / 3.3f);
    
    Serial.printf("[Sensors] MQ135 Raw ADC: %d, Voltage: %.3f V\n", rawMQ, mq135_v);

    record.mq135_raw = rawMQ;
    record.mq135_v = mq135_v; // Save the reconstructed sensor voltage for display/logging

    // Relative indicator used when no certified gas reference is available.
    // It intentionally avoids presenting the MQ135 as a selective CO2 instrument.
    float gasSpan = MQ135_THRESHOLD_VOLT - MQ135_BASELINE_VOLT;
    record.gas_index = gasSpan > 0.0
        ? 100.0 * (mq135_v - MQ135_BASELINE_VOLT) / gasSpan
        : NAN;
    if (!isnan(record.gas_index)) {
        record.gas_index = constrain(record.gas_index, 0.0f, MQ135_INDEX_MAX);
    }

    if (mqAdcMillivolts < 5.0f) {
        record.mq135_v = NAN;
        record.mq135_ppm = NAN;
        record.gas_index = NAN;
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

    // 3. Read GP2Y over multiple complete optical cycles. A trimmed mean is
    // less sensitive to fan/relay/ADC spikes than the former single reading.
    int rawDust = readGP2Y_TrimmedAverage();

    float dustAdcVoltage = rawDust * 3.3 / 4095.0;
    float dustSensorVoltage = dustAdcVoltage * VOLTAGE_DIVIDER_FACTOR;

    record.dust_raw = rawDust;
    record.dust_sensor_v = dustSensorVoltage;

    // Relative dust estimate. V0 and slope remain provisional until a
    // co-located reference instrument is available.
    float dust = (dustSensorVoltage - GP2Y_ZERO_DUST_VOLTAGE)
        * GP2Y_SLOPE_UG_M3_PER_V;
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

int Sensors::readGP2Y_TrimmedAverage() {
    int samples[GP2Y_SAMPLE_COUNT];

    for (int i = 0; i < GP2Y_SAMPLE_COUNT; i++) {
        digitalWrite(GP2Y_LED_PIN, LOW);
        delayMicroseconds(280);
        samples[i] = analogRead(GP2Y_PIN);
        delayMicroseconds(40);
        digitalWrite(GP2Y_LED_PIN, HIGH);
        delayMicroseconds(9680);
    }

    // Small fixed-size insertion sort avoids dynamic allocation on the MCU.
    for (int i = 1; i < GP2Y_SAMPLE_COUNT; i++) {
        int value = samples[i];
        int j = i - 1;
        while (j >= 0 && samples[j] > value) {
            samples[j + 1] = samples[j];
            j--;
        }
        samples[j + 1] = value;
    }

    long sum = 0;
    const int first = GP2Y_TRIM_COUNT;
    const int last = GP2Y_SAMPLE_COUNT - GP2Y_TRIM_COUNT;
    for (int i = first; i < last; i++) {
        sum += samples[i];
    }
    return sum / (last - first);
}
