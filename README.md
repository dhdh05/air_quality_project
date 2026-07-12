# HỆ THỐNG GIÁM SÁT CHẤT LƯỢNG KHÔNG KHÍ (ESP32 AIR QUALITY MONITOR)

Dự án này là firmware nguồn mở được viết bằng C++ trên nền tảng **PlatformIO (VS Code)** cho vi điều khiển ESP32. Hệ thống sử dụng máy trạng thái bất đồng bộ (non-blocking state machine) để quản lý điện năng hiệu quả thông qua MOSFET/Relay, giám sát chất lượng không khí qua các cảm biến khí gas, bụi mịn, nhiệt độ/độ ẩm, và ghi nhật ký dữ liệu thời gian thực lên cơ sở dữ liệu đám mây **Supabase** qua các API RESTful.

Hệ thống còn tích hợp cơ chế cập nhật từ xa (OTA), cơ chế tự động khởi động lại khi treo thông qua Watchdog Timer phần cứng, và nút nhấn vật lý hỗ trợ ngắt để đánh thức giao diện hoặc tắt còi báo động.

---

## 1. Tổng Quan Kiến Trúc Hệ Thống (System Architecture)

Hệ thống hoạt động dựa trên mô hình máy trạng thái (FSM) gồm 3 trạng thái hoạt động độc lập nhằm tiết kiệm điện năng cho lò sưởi MQ135 và kéo dài tuổi thọ của hệ thống:

```
                  ┌────────────────────────┐
                  │        Trạng thái      │
                  │       IDLE (5 phút)    │◄────────────────────────┐
                  │ - Ngắt nguồn MOSFET    │                         │
                  │ - OLED tắt sau 30 giây │                         │
                  └───────────┬────────────┘                         │
                              │                                      │
                              │ (Đủ 5 phút)                          │
                              ▼                                      │
                  ┌────────────────────────┐                         │
                  │        Trạng thái      │                         │
                  │     WARMUP (60 giây)   │                         │
                  │ - Đóng nguồn MOSFET    │                         │
                  │ - Khởi động lò sưởi    │                         │
                  └───────────┬────────────┘                         │
                              │                                      │
                              │ (Đủ 60 giây)                         │
                              ▼                                      │
                  ┌────────────────────────┐                         │
                  │       Trạng thái       │                         │
                  │   READ & PUBLISH       │─────────────────────────┘
                  │ - Đọc cảm biến         │ (Xử lý xong)
                  │ - Cập nhật màn hình    │
                  │ - POST lên Supabase    │
                  │ - Ngắt nguồn MOSFET    │
                  └────────────────────────┘
```

*   **IDLE (Chờ)**: MOSFET tắt nguồn cấp 5V cho cảm biến MQ135 (bộ nung nóng) và quạt hút. Màn hình OLED SSD1306 tự động chuyển sang chế độ ngủ (sleep mode) sau 30 giây để chống hiện tượng burn-in (lưu ảnh). Chu kỳ chờ mặc định là 5 phút (`SAMPLING_INTERVAL`).
*   **WARMUP (Làm nóng)**: MOSFET được kích dẫn để cấp nguồn 5V. Hệ thống chờ trong 60 giây (`WARMUP_DURATION`) để lò sưởi của MQ135 ổn định trước khi tiến hành đo đạc chính xác. Màn hình OLED tự động bật để báo tiến trình.
*   **READ_AND_PUBLISH (Đo & Gửi dữ liệu)**: ESP32 tiến hành lấy mẫu cảm biến, hiển thị kết quả lên OLED, đẩy payload JSON tới Supabase API qua Wi-Fi, kích hoạt cảnh báo (LED/Buzzer) nếu chất lượng không khí vượt ngưỡng, sau đó ngắt nguồn MOSFET và quay lại trạng thái IDLE.

---

## 2. Hướng Dẫn Nối Dây Phần Cứng (Hardware Wiring Guide)

Hệ thống hoạt động với hai đường nguồn riêng biệt: nguồn USB **3.3V - 5V từ ESP32** cho các linh kiện logic dòng nhỏ, và nguồn ngoài độc lập **5V - 2A** cấp cho lò sưởi MQ135 và quạt gió nhằm tránh hiện tượng sụt áp hệ thống gây reset vi điều khiển.

### Sơ đồ đấu nối MOSFET/Relay điều khiển nguồn
Cổng Gate của MOSFET (hoặc chân tín hiệu Relay) được điều khiển thông qua chân `SENSOR_POWER_PIN` (GPIO 32). Khi ở trạng thái `WARMUP` và `READ_AND_PUBLISH`, chân này xuất mức cao (`HIGH`) để dẫn thông nguồn 5V ngoài cấp cho chân **VCC của MQ135** và cực dương của **Quạt gió**.

### Bảng cấu hình sơ đồ chân (Pin Mapping)

| Thiết bị | Chân thiết bị | Chân ESP32 | Ghi chú / Yêu cầu phần cứng |
| :--- | :--- | :--- | :--- |
| **Relay / MOSFET** | Chân điều khiển (Gate/Signal) | **GPIO 16** | Điều khiển đóng/cắt nguồn cấp 5V ngoài cho MQ135 và Quạt |
| **Màn hình OLED** | SDA | **GPIO 21** | Giao tiếp I2C, cần nguồn 3.3V |
| | SCL | **GPIO 22** | Giao tiếp I2C, cần nguồn 3.3V |
| **DHT22** | Data | **GPIO 14** | Cần kết nối điện trở pull-up 4.7kΩ - 10kΩ giữa Data và VCC |
| **MQ135** | Analog Out (Ao) | **GPIO 34** | Đọc giá trị ADC, qua cầu chia áp từ 5V về 3.3V (R1=12kΩ, R2=22kΩ) |
| **GP2Y1010F** | LED Control | **GPIO 25** | Tín hiệu điều khiển LED bên trong cảm biến bụi (Active LOW) |
| | Analog Out (Vo) | **GPIO 35** | Tọc giá trị bụi qua cầu chia áp (giống MQ135) |
| **Buzzer** | Chân tín hiệu | **GPIO 27** | Còi cảnh báo mức cao (Active HIGH) |
| **LED Cảnh Báo** | Cực dương (Anode) | **GPIO 26** | Đèn LED cảnh báo chất lượng không khí kém |
| **Nút Nhấn UI** | Chân tín hiệu | **GPIO 4** | Cấu hình ngắt `INPUT_PULLUP`. Nhấn để đánh thức OLED/Tắt còi tạm thời |

> [!WARNING]
> Không được nối trực tiếp cổng 5V của cảm biến MQ135 hoặc Quạt vào chân nguồn cấp của ESP32. Bộ nguồn 5V-2A ngoài phải được sử dụng chung GND (Ground) với mạch ESP32.

---

## 3. Cài Đặt Phần Mềm (Software Setup with PlatformIO)

### Yêu cầu hệ thống:
1.  **VS Code** đã cài đặt extension **PlatformIO IDE**.
2.  Máy tính có cổng kết nối nạp code cho ESP32.

### Các bước cài đặt:
1.  Mở dự án bằng cách chọn **File** -> **Open Folder...** và trỏ đến thư mục chứa mã nguồn.
2.  PlatformIO sẽ tự động tải xuống các thư viện phụ thuộc đã được định cấu hình trong tệp `platformio.ini`, bao gồm:
    *   `Adafruit SSD1306` (Màn hình OLED)
    *   `Adafruit GFX Library` (Đồ họa OLED)
    *   `DHT sensor library` (Cảm biến DHT22)
    *   `Adafruit Unified Sensor`
    *   `ArduinoJson` (Xử lý chuỗi JSON gửi đi)
3.  Biên dịch dự án bằng cách nhấp vào biểu tượng **Build** (dấu tích) ở thanh công cụ PlatformIO phía dưới màn hình.

---

## 4. Bảo Mật & Cấu Hình Thông Tin Kết Nối (secrets.h)

Để tránh rò rỉ mật khẩu WiFi và mã bảo mật cơ sở dữ liệu khi đẩy mã nguồn lên GitHub, thông tin xác thực phải được lưu trữ trong một tệp tin cấu hình bảo mật riêng.

### Bước 1: Tạo file cấu hình
Hãy tạo một tệp tin có tên `secrets.h` đặt trong đường dẫn `firmware/include/secrets.h` với nội dung mẫu bên dưới:

```cpp
#pragma once

// =========================================================================
// WARNING: DO NOT COMMIT THIS FILE TO VERSION CONTROL.
// Make sure to add "firmware/include/secrets.h" to your .gitignore file.
// =========================================================================

// Wi-Fi Access Point Credentials
#define WIFI_SSID       "TÊN_WIFI_CỦA_BẠN"
#define WIFI_PASS       "MẬT_KHẨU_WIFI_CỦA_BẠN"

// Supabase REST Endpoint and API keys
#define SUPABASE_URL    "https://your-project-id.supabase.co/rest/v1/air_quality_logs"
#define SUPABASE_KEY    "MÃ_BẢO_MẬT_ANON_HOẶC_SERVICE_ROLE_KEY_TRÊN_SUPABASE"
```

### Bước 2: Thiết lập Git Ignore
Đảm bảo tệp tin `.gitignore` tại thư mục gốc của bạn đã chứa đường dẫn tới file bí mật này để tránh bị commit ngoài ý muốn:

```text
firmware/include/secrets.h
```

---

## 5. Cấu Hình Cơ Sở Dữ Liệu Supabase Backend

Dữ liệu cảm biến được truyền tải lên Supabase REST API dưới dạng một dòng dữ liệu JSON. Bạn cần tạo một bảng cơ sở dữ liệu trên Supabase để lưu trữ dữ liệu này.

### Các bước thực hiện:
1.  Đăng nhập vào tài khoản [Supabase](https://supabase.com/).
2.  Tạo một Project mới và thiết lập mật khẩu cơ sở dữ liệu.
3.  Vào mục **SQL Editor** trong thanh công cụ bên trái của trang quản trị Supabase.
4.  Dán và chạy khối lệnh SQL dưới đây để tạo bảng `air_quality_logs` với các cột tương ứng phù hợp cấu trúc dữ liệu JSON được gửi từ thiết bị:

```sql
-- Tạo bảng lưu trữ nhật ký chất lượng không khí
CREATE TABLE air_quality_logs (
    id bigint GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    created_at timestamp with time zone DEFAULT timezone('utc'::text, now()) NOT NULL,
    temperature numeric,
    humidity numeric,
    mq135_raw integer,
    mq135_v numeric,
    dust_density numeric
);

-- Cho phép quyền truy cập bảng thông qua REST API (Anon Key)
ALTER TABLE air_quality_logs ENABLE ROW LEVEL SECURITY;

-- Tạo chính sách cho phép bất kỳ ai có Anon Key cũng có thể ghi (INSERT) dữ liệu vào bảng
CREATE POLICY "Cho phép ghi dữ liệu tự động" ON air_quality_logs
    FOR INSERT 
    WITH CHECK (true);
```

5.  Lấy thông tin **Project URL** và **API key (anon public)** từ trang cài đặt dự án (*Project Settings* -> *API*) và cập nhật chúng vào file `secrets.h`.

---

## 6. Các Tính Năng Nâng Cao (Advanced Features)

*   **Ngắt Nút Nhấn Vật Lý**: Nút nhấn trên `BUTTON_PIN` (GPIO 33) cấu hình chế độ ngắt cạnh xuống (`FALLING`). Khi được nhấn, ngắt phần cứng ghi nhận cờ tín hiệu tức thì giúp đánh thức màn hình OLED SSD1306 đang ngủ, đồng thời tắt âm còi báo động (Mute Buzzer) nếu hệ thống đang ở trạng thái cảnh báo khói.
*   **Watchdog Timer (WDT) Phần Cứng**: Khởi động WDT của ESP32 với chu kỳ 15 giây. Trong các tác vụ tốn thời gian như thiết lập WiFi và đăng ký gửi gói tin HTTP POST tới Supabase, hàm reset watchdog được tích hợp trong vòng lặp kết nối để ngăn ngừa ESP32 bị reset nhầm, đồng thời bảo vệ hệ thống không bị treo vô hạn khi mất tín hiệu mạng đột ngột.
*   **Cập Nhật Từ Xa (OTA Updates)**: Chức năng `setupOTA()` được tích hợp sẵn giúp nạp firmware mới qua giao thức mạng nội bộ không dây (WiFi), loại bỏ sự bất tiện của việc phải tháo dỡ thiết bị để cắm cáp nạp trực tiếp.
*   **Cảnh Báo Từ Xa qua Telegram Bot**: Hỗ trợ Supabase Edge Function cho Telegram alert; cần cấu hình secret và deploy function. Đây là tính năng chạy phía Cloud để bảo mật thông tin, hoàn toàn không lưu trữ token Telegram trên firmware ESP32.

