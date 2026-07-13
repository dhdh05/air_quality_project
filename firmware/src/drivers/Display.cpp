#include "Display.h"
#include <Wire.h>

Display::Display() : _display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {}

bool Display::begin() {
    // Initialise the OLED display using I2C address
    if (!_display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        return false;
    }
    
    _display.clearDisplay();
    _display.setTextColor(SSD1306_WHITE);
    _display.setTextSize(1);
    _display.display();
    return true;
}

void Display::showSplash() {
    _display.clearDisplay();
    _display.setTextColor(SSD1306_WHITE);
    
    _display.setTextSize(1);
    _display.setCursor(0, 0);
    _display.println("AIR QUALITY MONITOR");
    _display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
    
    _display.setCursor(0, 16);
    _display.println("Initializing system...");
    _display.setCursor(0, 28);
    _display.println("- DHT22 Temp/Humid");
    _display.println("- MQ135 Gas Sensor");
    _display.println("- GP2Y1010F Dust");
    _display.println("- WiFi & Supabase API");
    _display.display();
}

void Display::showStatus(const char* status) {
    _display.clearDisplay();
    _display.setTextColor(SSD1306_WHITE);
    
    _display.setTextSize(1);
    _display.setCursor(0, 0);
    _display.println("AIR QUALITY MONITOR");
    _display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
    
    _display.setCursor(0, 24);
    _display.println(status);
    
    _display.display();
}


void Display::showData(const SensorRecord& data, const char* status) {
    _display.clearDisplay();
    _display.setTextColor(SSD1306_WHITE);
    _display.setTextSize(1);
    
    // Header containing state info
    _display.setCursor(0, 0);
    _display.print("AQ METER [");
    _display.print(status);
    _display.println("]");
    _display.drawFastHLine(0, 8, SCREEN_WIDTH, SSD1306_WHITE);
    
    // Temperature and Humidity
    _display.setCursor(0, 12);
    _display.print("T: ");
    if (isnan(data.temperature) || data.status == SENSOR_READ_ERROR) {
        _display.print("--");
    } else {
        _display.print(data.temperature, 1);
        _display.print(" C");
    }

    _display.setCursor(70, 12);
    _display.print("H: ");
    if (isnan(data.humidity) || data.status == SENSOR_READ_ERROR) {
        _display.print("--");
    } else {
        _display.print(data.humidity, 0);
        _display.print("%");
    }

    // MQ135 Gas readings in mV
    _display.setCursor(0, 25);
    _display.print("Gas: ");
    if (isnan(data.mq135_v) || data.status == SENSOR_READ_ERROR) {
        _display.print("--");
    } else {
        _display.print(data.mq135_v * 1000.0, 0);
        _display.print(" mV");
    }

    // Relative gas index. This avoids claiming selective/certified CO2 data.
    _display.setCursor(0, 37);
    _display.print("Gas idx: ");
    if (isnan(data.gas_index) || data.status == SENSOR_READ_ERROR) {
        _display.print("--");
    } else {
        _display.print(data.gas_index, 0);
        _display.print("/100");
    }

    // GP2Y Dust Density
    _display.setCursor(0, 49);
    _display.print("Dust: ");
    if (data.status == SENSOR_READ_ERROR) {
        _display.print("--");
    } else {
        _display.print(data.dust_density, 0);
        _display.print(" ug/m3");
    }

    // Display status flag
    _display.setCursor(85, 49);
    if (data.status == SENSOR_OK) {
        _display.print("[OK]");
    } else if (data.status == OFFLINE_CACHED) {
        _display.print("[CACHE]");
    } else {
        _display.print("[ERR]");
    }

    // Display ALARM badge if warning active and status is not read error
    if (data.warning && data.status != SENSOR_READ_ERROR) {
        _display.fillRect(85, 23, 43, 12, SSD1306_WHITE);
        _display.setTextColor(SSD1306_BLACK);
        _display.setCursor(89, 25);
        _display.print("ALARM");
        _display.setTextColor(SSD1306_WHITE); // Restore text color
    }

    _display.display();
}


void Display::clear() {
    _display.clearDisplay();
    _display.display();
}

void Display::sleep() {
    _display.ssd1306_command(SSD1306_DISPLAYOFF);
}

void Display::wakeUp() {
    _display.ssd1306_command(SSD1306_DISPLAYON);
}
