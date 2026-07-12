# TÀI LIỆU KHỚP NỐI DỮ LIỆU CẢM BIẾN (FIRMWARE & CLOUD PAYLOAD ALIGNMENT)

Tài liệu này đối chiếu và xác nhận sự trùng khớp về cấu trúc dữ liệu gửi đi từ vi điều khiển ESP32 (Firmware) và cấu trúc dữ liệu nhận được tại Supabase Edge Function.

---

## 1. So Sánh Các Trường Dữ Liệu (Field Mapping)

Dưới đây là bảng đối chiếu chi tiết giữa cấu trúc `SensorRecord` trong firmware, Payload JSON được gửi qua HTTP POST từ ESP32, cấu trúc bảng `air_quality_logs` trong Supabase và các trường kiểm tra trong Edge Function.

| Trường trên ESP32 (`SensorRecord`) | Trường gửi đi (JSON Payload) | Kiểu dữ liệu trong DB | Biên dịch trong Edge Function | Ghi chú |
| :--- | :--- | :--- | :--- | :--- |
| `temperature` | `temperature` | `numeric` | `record.temperature` | Nhiệt độ (°C) từ DHT22 |
| `humidity` | `humidity` | `numeric` | `record.humidity` | Độ ẩm (%) từ DHT22 |
| `mq135_ppm` | `mq135_ppm` | `numeric` | `record.mq135_ppm` | Nồng độ khí gas (PPM) |
| `mq135_v` | `mq135_v` | `numeric` | `record.mq135_v` | Điện áp thô (V) của MQ135 |
| `dust_density` | `dust_density` | `numeric` | `record.dust_density` | Mật độ bụi mịn (ug/m3) từ GP2Y |
| `status` | `status` | `integer` | `record.status` | Trạng thái (0: OK, 1: Lỗi, 2: Timeout, 3: Cached) |
| `warning` | `warning` | `boolean` | `record.warning` | Cảnh báo cục bộ của thiết bị |

---

## 2. Điểm Lưu Ý Về Việc Đồng Bộ Trạng Thái `status`
- Firmware định nghĩa trạng thái truyền tin offline thông qua giá trị `status` bằng `OFFLINE_CACHED` (mã giá trị `3`).
- **Edge Function** đã được lập trình để bỏ qua cảnh báo đối với giá trị trạng thái `3` (`OFFLINE_CACHED`) nhằm tránh việc gửi hàng loạt thông báo cảnh báo lỗi giả khi thiết bị kết nối mạng trở lại và đẩy các bản ghi lưu trữ tạm thời trong bộ nhớ SPIFFS lên hệ thống.
- Cảnh báo lỗi cảm biến/hệ thống chỉ được kích hoạt khi `status` có giá trị là `1` (`SENSOR_READ_ERROR`) hoặc `2` (`SENSOR_TIMEOUT`).

---

## 3. Khớp Nối SQL DDL Cho Bảng Cơ Sở Dữ Liệu
Để đảm bảo cơ sở dữ liệu chấp nhận toàn bộ các cột được gửi lên từ thiết bị, bảng `air_quality_logs` cần được bổ sung cấu trúc như sau:

```sql
-- Cập nhật cột nếu bảng đã tồn tại từ trước
ALTER TABLE air_quality_logs ADD COLUMN IF NOT EXISTS mq135_ppm numeric;
ALTER TABLE air_quality_logs ADD COLUMN IF NOT EXISTS mq135_v numeric;
ALTER TABLE air_quality_logs ADD COLUMN IF NOT EXISTS status integer;
ALTER TABLE air_quality_logs ADD COLUMN IF NOT EXISTS warning boolean;
```

*Nhận xét: Cơ chế dữ liệu hiện tại giữa thiết bị và đám mây đã hoàn toàn đồng bộ, không phát sinh bất kỳ xung đột trường dữ liệu nào.*
