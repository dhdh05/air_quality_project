# Supabase Edge Functions - Telegram Alert WARNING System

Thư mục này chứa mã nguồn và tài liệu cấu hình cho hệ thống cảnh báo từ xa thông qua **Telegram Bot** sử dụng **Supabase Edge Functions**.

## 1. Mục Đích (Purpose)
Chức năng gửi cảnh báo Telegram Bot được triển khai ở phía **Cloud (Supabase Edge Function)** thay vì nằm trên firmware của ESP32. 
- **Bảo mật**: Tránh việc phải nhúng và lộ mã token của Telegram Bot (`TELEGRAM_BOT_TOKEN`) trên bộ nhớ Flash của ESP32.
- **Tối ưu hóa tài nguyên**: ESP32 chỉ cần thực hiện kết nối Wi-Fi thông thường và gọi một API REST duy nhất (HTTP POST gửi dữ liệu lên Supabase). Supabase sẽ đảm nhiệm xử lý logic kiểm tra ngưỡng cảnh báo phức tạp và liên kết với dịch vụ của bên thứ ba.

---

## 2. Cấu Trúc Thư Mục `supabase/`
```text
supabase/
├── functions/
│   └── telegram-alert/
│       └── index.ts       # Mã nguồn Deno / TypeScript xử lý cảnh báo
├── .env.example           # File cấu hình biến môi trường mẫu
└── README.md              # Tài liệu hướng dẫn sử dụng (file này)
```

---

## 3. Các Biến Môi Trường Cần Cấu Hình (Secrets)
Bạn cần thiết lập các biến môi trường này trên Supabase để chức năng hoạt động chính xác.

| Tên biến | Kiểu dữ liệu | Ý nghĩa / Mẫu |
| :--- | :--- | :--- |
| `TELEGRAM_BOT_TOKEN` | String | Token của Telegram Bot nhận từ `@BotFather` |
| `TELEGRAM_CHAT_ID` | String/Number | ID chat của người dùng hoặc nhóm nhận cảnh báo |
| `MQ135_ALERT_THRESHOLD` | Number | Ngưỡng cảnh báo khí gas MQ135 (PPM), mặc định: `1000` |
| `DUST_ALERT_THRESHOLD` | Number | Ngưỡng cảnh báo mật độ bụi (ug/m3), mặc định: `150` |
| `TEMP_ALERT_THRESHOLD` | Number | Ngưỡng cảnh báo nhiệt độ cao (°C), mặc định: `40` |
| `HUMIDITY_ALERT_THRESHOLD` | Number | Ngưỡng cảnh báo độ ẩm cao (%), mặc định: `85` |

---

## 4. Hướng Dẫn Deploy (Deployment Guide)

### Bước 1: Đăng nhập và liên kết dự án (Authentication & Link)
Để deploy được lên hệ thống Supabase, bạn cần đăng nhập tài khoản và liên kết CLI với project của bạn:

1. Đăng nhập CLI:
   ```bash
   npx supabase login
   ```
   *(Lệnh này sẽ mở trình duyệt để bạn đăng nhập hoặc nhập Access Token được tạo tại Supabase Dashboard -> Account).*

2. Liên kết CLI với dự án của bạn (Project ID của bạn là `ggmcdxzzxxqonanyrsii`):
   ```bash
   npx supabase link --project-ref ggmcdxzzxxqonanyrsii
   ```
   *(CLI sẽ yêu cầu nhập mật khẩu cơ sở dữ liệu Database Password mà bạn đặt lúc tạo project).*

### Bước 2: Deploy Edge Function
Sau khi đăng nhập và liên kết thành công, đẩy hàm `telegram-alert` lên Cloud:
```bash
npx supabase functions deploy telegram-alert
```

### Bước 2: Thiết Lập Các Khoá Secrets/Token
Chạy các dòng lệnh dưới đây để nạp biến môi trường cho dự án Supabase:
```bash
npx supabase secrets set TELEGRAM_BOT_TOKEN="your_bot_token"
npx supabase secrets set TELEGRAM_CHAT_ID="your_chat_id"
npx supabase secrets set MQ135_ALERT_THRESHOLD="1000"
npx supabase secrets set DUST_ALERT_THRESHOLD="150"
npx supabase secrets set TEMP_ALERT_THRESHOLD="40"
npx supabase secrets set HUMIDITY_ALERT_THRESHOLD="85"
```

---

## 5. Cấu Trúc Payload Dự Kiến (Expected Payload)
Supabase Database Webhook sẽ gửi thông tin bản ghi vừa được `INSERT` vào bảng dưới định dạng JSON sau:

```json
{
  "type": "INSERT",
  "table": "air_quality_logs",
  "schema": "public",
  "record": {
    "id": 42,
    "created_at": "2026-07-12T00:05:00Z",
    "temperature": 41.5,
    "humidity": 60.0,
    "mq135_ppm": 1200.5,
    "mq135_v": 2.85,
    "dust_density": 165.2,
    "status": 0,
    "warning": false
  },
  "old_record": null
}
```

---

## 6. Lệnh curl Để Test Nhập Thử Cảnh Báo (Testing)

Bạn có thể chạy thử lệnh curl dưới đây (sau khi đã cập nhật đúng đường dẫn URL của Edge Function và khoá `Authorization` là Bearer của `anon` hoặc `service_role` key) để kiểm tra phản hồi từ Edge Function:

```bash
curl -i --location --request POST 'https://your-project-id.supabase.co/functions/v1/telegram-alert' \
--header 'Content-Type: application/json' \
--header 'Authorization: Bearer YOUR_SUPABASE_ANON_KEY' \
--data-raw '{
  "type": "INSERT",
  "table": "air_quality_logs",
  "schema": "public",
  "record": {
    "id": 1,
    "created_at": "2026-07-12T00:00:00Z",
    "temperature": 42.0,
    "humidity": 65.0,
    "mq135_ppm": 1100,
    "mq135_v": 2.80,
    "dust_density": 80.5,
    "status": 0,
    "warning": false
  }
}'
```

### Kết Quả Trả Về Dự Kiến:
- **Nếu vượt ngưỡng và gửi Telegram thành công**:
  ```json
  { "alert": true, "telegram": "sent" }
  ```
- **Nếu các chỉ số an toàn (không vượt ngưỡng)**:
  ```json
  { "alert": false, "message": "No alert condition matched" }
  ```
