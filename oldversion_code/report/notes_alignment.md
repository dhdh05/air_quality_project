# Nhật ký Đối Khớp Kỹ Thuật (Report-to-Code Alignment Log)

Tài liệu này ghi nhận các thông số kỹ thuật được trích xuất trực tiếp từ mã nguồn nhúng hiện tại để đưa vào báo cáo LaTeX nhằm đảm bảo tính đồng nhất tuyệt đối giữa lý thuyết báo cáo và thực tế triển khai bo mạch.

## 1. Các Thông Số Kỹ Thuật Trích Từ Mã Nguồn (Config.h)

- **Cấu hình GPIO**:
  - DHT22 (Nhiệt/Ẩm): `GPIO 14`
  - MQ135 (Analog): `GPIO 34`
  - GP2Y1010AU0F Analog: `GPIO 35`
  - GP2Y1010AU0F LED Control: `GPIO 25` (Mức logic tích cực thấp)
  - WARNING LED: `GPIO 26`
  - BUZZER: `GPIO 27`
  - RELAY (Quạt buồng đo): `GPIO 16`
  - BUTTON (Ngắt ngoài UI): `GPIO 4`
  - I2C: SDA=`GPIO 21`, SCL=`GPIO 22`
- **Mạch cầu phân áp MQ135**:
  - $R1 = 12k\Omega$
  - $R2 = 22k\Omega$
  - Hệ số chênh lệch quy đổi điện áp: `VOLTAGE_DIVIDER_FACTOR = 1.545`
  - Điện áp nung và so sánh: $5.0V$, Ngưỡng cảnh báo: $2.74V$
- **Hệ số cảm biến bụi GP2Y**:
  - Điện áp điểm không (Zero dust): `0.588V`
  - Độ nhạy (Sensitivity): `0.17V per 100 ug/m3`

## 2. Các Task FreeRTOS & Mutex Được Đồng Bộ (main.cpp)

- **TaskSensors**: Đọc cảm biến, chạy State Machine (Core 1, Priority 2, Stack 4096).
- **TaskOLED**: Quét hiển thị OLED (Core 1, Priority 1, Stack 4096).
- **TaskNetwork**: Quản lý kết nối Wi-Fi, upload dữ liệu REST API và Cache SPIFFS (Core 0, Priority 1, Stack 8192).
- **Watchdog**: Sử dụng Hardware Watchdog 15 giây.
- **Mutexes**: 
  - `dataMutex` bảo vệ cấu trúc SensorRecord.
  - `i2cMutex` bảo vệ bus I2C SDA/SCL.

## 3. Các Điểm Khác Biệt Giữa Mã Nguồn Và Tài Liệu Nhật Ký Cũ

Trong quá trình rà soát, nhóm phát hiện một số sai khác nhỏ giữa tài liệu nháp cũ của các thành viên trước đó và mã nguồn thực tế:
- *Địa chỉ I2C*: Nhật ký nháp cũ ghi OLED địa chỉ `0x3C` hoặc `0x3D`. Trong code hiện tại, OLED được thiết lập cứng ở địa chỉ `0x3C`. Báo cáo đã thống nhất ghi `0x3C`.
- *Ngưỡng cảnh báo bụi*: Cấu hình ngưỡng cảnh báo trong code nhúng kích hoạt dựa trên cảnh báo chung `warning` từ sensors, báo cáo đã mô tả rõ ngưỡng cảnh báo bụi thô ở mức $150 \mu g/m^3$ ngoài trời và MQ135 ở mức $1000$ PPM.

## 4. Danh Sách Các Ảnh Đồ Họa Đã Có (Và Có Thể Thay Thế)

Để báo cáo biên dịch thành công và có giao diện đồ họa trực quan, tôi đã tạo sẵn các ảnh đồ họa thiết kế vector/3D trong thư mục `images/` trên Overleaf:
1. `logo_hust.png`: Ảnh biểu trưng thiết kế vector cho trường (dùng ở trang bìa).
2. `wiring_detail.png`: Sơ đồ nguyên lý nối chân logic giữa ESP32 và cảm biến Sharp GP2Y/các cảm biến khác (dùng ở Chương 3).
3. `prototype_overview.png`: Ảnh dựng 3D mô phỏng mô hình nguyên mẫu trạm đo thực tế đặt trong hộp acrylic (dùng ở Chương 4).

*(Lưu ý: Các hình ảnh trên là ảnh dựng vector/3D minh họa độ phân giải cao đã được tạo sẵn trong thư mục `images/` để đảm bảo tài liệu biên dịch thành công. Bạn hoàn toàn có thể thay thế bằng các ảnh chụp thực tế hoặc sơ đồ vẽ mạch Altium của nhóm bằng cách lưu đè tệp tin trùng tên vào thư mục `images/` trên Overleaf).*
