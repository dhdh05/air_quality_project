#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ================= DHT22 =================
#define DHTPIN 14
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ================= SENSOR PIN =================
#define MQ135_PIN 34
#define GP2Y_PIN 35
#define GP2Y_LED_PIN 25

// ================= OUTPUT =================
#define WARNING_LED_PIN 26
#define BUZZER_PIN 27

// Chia áp R1 = 12k, R2 = 22k
// V_ESP32 = V_sensor * 22 / (12 + 22)
// V_sensor = V_ESP32 * 1.545
#define VOLTAGE_DIVIDER_FACTOR 1.545

// Ngưỡng cảnh báo MQ135
// Nếu không khí bình thường mà đã kêu thì tăng số này lên
// Nếu muốn nhạy hơn thì giảm xuống
int MQ135_THRESHOLD = 2200;

// ================= HÀM ĐỌC ADC TRUNG BÌNH =================
int readADC_Average(int pin, int samples = 20) {
  long sum = 0;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(2);
  }

  return sum / samples;
}

// ================= HÀM ĐỌC BỤI GP2Y1010F =================
float readDustGP2Y() {
  // GP2Y LED active LOW
  digitalWrite(GP2Y_LED_PIN, LOW);     // bật LED trong cảm biến
  delayMicroseconds(280);

  int raw = analogRead(GP2Y_PIN);

  delayMicroseconds(40);
  digitalWrite(GP2Y_LED_PIN, HIGH);    // tắt LED trong cảm biến
  delayMicroseconds(9680);

  float adcVoltage = raw * 3.3 / 4095.0;
  float sensorVoltage = adcVoltage * VOLTAGE_DIVIDER_FACTOR;

  // Công thức gần đúng cho GP2Y1010F
  float dustDensity = (0.17 * sensorVoltage - 0.1) * 1000.0;

  if (dustDensity < 0) {
    dustDensity = 0;
  }

  return dustDensity;
}

void setup() {
  Serial.begin(115200);

  pinMode(WARNING_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(GP2Y_LED_PIN, OUTPUT);

  digitalWrite(WARNING_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(GP2Y_LED_PIN, HIGH);  // tắt LED GP2Y ban đầu

  // ADC ESP32
  analogReadResolution(12);
  analogSetPinAttenuation(MQ135_PIN, ADC_11db);
  analogSetPinAttenuation(GP2Y_PIN, ADC_11db);

  // I2C OLED
  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("Khong tim thay OLED!");
    while (1);
  }

  dht.begin();

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("AIR QUALITY");
  display.println("ESP32 + MQ135");
  display.println("DHT22 + GP2Y1010F");
  display.display();

  delay(2000);
}

void loop() {
  // Đọc DHT22
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Đọc MQ135
  int mqRaw = readADC_Average(MQ135_PIN, 30);
  float mqAdcVoltage = mqRaw * 3.3 / 4095.0;
  float mqSensorVoltage = mqAdcVoltage * VOLTAGE_DIVIDER_FACTOR;

  // Đọc bụi
  float dust = readDustGP2Y();

  // Kiểm tra cảnh báo
  bool warning = mqRaw > MQ135_THRESHOLD;

  if (warning) {
    digitalWrite(WARNING_LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(WARNING_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }

  // Serial Monitor
  Serial.print("Nhiet do: ");
  Serial.print(temperature);
  Serial.print(" C | Do am: ");
  Serial.print(humidity);
  Serial.print(" % | MQ135 raw: ");
  Serial.print(mqRaw);
  Serial.print(" | MQ135 V: ");
  Serial.print(mqSensorVoltage, 2);
  Serial.print(" V | Bui: ");
  Serial.print(dust, 0);
  Serial.println(" ug/m3");

  // OLED
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("AIR QUALITY METER");

  display.setCursor(0, 12);
  display.print("T: ");
  if (isnan(temperature)) {
    display.print("--");
  } else {
    display.print(temperature, 1);
  }
  display.print(" C");

  display.setCursor(70, 12);
  display.print("H: ");
  if (isnan(humidity)) {
    display.print("--");
  } else {
    display.print(humidity, 0);
  }
  display.print("%");

  display.setCursor(0, 25);
  display.print("MQ135: ");
  display.print(mqRaw);

  display.setCursor(0, 37);
  display.print("Gas V: ");
  display.print(mqSensorVoltage, 2);
  display.print("V");

  display.setCursor(0, 49);
  display.print("Dust: ");
  display.print(dust, 0);
  display.print(" ug/m3");

  if (warning) {
    display.fillRect(85, 25, 43, 12, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(89, 27);
    display.print("ALARM");
    display.setTextColor(SSD1306_WHITE);
  }

  display.display();

  delay(1000);
}