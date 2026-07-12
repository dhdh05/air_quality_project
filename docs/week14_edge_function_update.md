# NHẬT KÝ KỸ THUẬT - TUẦN 14
## Nâng Cấp Hệ Thống Cảnh Báo Từ Xa Qua Telegram Bot & Supabase Edge Functions

### 1. Nội dung thực hiện
Trong tuần 14, nhóm đã tiến hành thiết kế, lập trình và tích hợp thành công cơ chế cảnh báo từ xa thông qua ứng dụng Telegram bên cạnh các kênh cảnh báo tại chỗ (LED/Còi buzzer). Việc gửi cảnh báo được xây dựng dựa trên mô hình phi tập trung sử dụng dịch vụ đám mây nhằm tối ưu hóa tài nguyên phần cứng và bảo mật thông tin.

### 2. Chi tiết giải pháp kỹ thuật
- **Vị trí xử lý cảnh báo (Cloud-side Alert)**:
  - Logic gửi tin nhắn cảnh báo qua Telegram Bot được triển khai hoàn toàn ở phía máy chủ (**Supabase Edge Functions** viết bằng TypeScript/Deno).
  - Quyết định này giúp bảo mật thông tin tuyệt đối do không cần phải lưu trữ mã thông tin nhạy cảm (`TELEGRAM_BOT_TOKEN`) trực tiếp trên firmware của vi điều khiển ESP32, hạn chế nguy cơ bị dịch ngược hoặc rò rỉ mã bảo mật.
- **Luồng hoạt động (Data Flow)**:
  1. Vi điều khiển ESP32 sau khi làm nóng và thu thập dữ liệu từ các cảm biến (DHT22, MQ135, GP2Y1010) sẽ đóng gói payload JSON và gửi yêu cầu RESTful HTTP POST trực tiếp lên cơ sở dữ liệu Supabase.
  2. Bảng `air_quality_logs` trên Supabase lưu trữ bản ghi dữ liệu mới.
  3. Một **Database Webhook** được đăng ký trên Supabase sẽ kích hoạt bất kỳ khi nào có thao tác `INSERT` một bản ghi chất lượng không khí mới. Webhook này thực hiện gọi HTTP POST chuyển tiếp bản ghi sang Edge Function `telegram-alert`.
  4. Edge Function phân tích các chỉ số nhận được so với các ngưỡng cảnh báo cấu hình linh hoạt qua biến môi trường (Environment Variables).
  5. Nếu bất kỳ chỉ số nào vượt ngưỡng hoặc phát hiện lỗi cảm biến, Edge Function sẽ đóng gói tin nhắn tiếng Việt không dấu và gọi Telegram Bot API (`sendMessage`) để phát tín hiệu cảnh báo tới điện thoại của người dùng.

### 3. Trạng thái triển khai
- **Mã nguồn firmware**: Đã cập nhật thành công việc đóng gói và gửi các thông tin bổ sung (`mq135_v`, `status`, `warning`) lên Supabase từ thiết bị.
- **Mã nguồn Edge Function**: Đã hoàn thiện cấu trúc thư mục dự án `supabase/` chứa mã nguồn TypeScript chạy trên Deno.
- **Trạng thái chạy thử**: Hiện tại mã nguồn và tài liệu hướng dẫn cấu hình chi tiết đã được bổ sung đầy đủ vào kho lưu trữ (repository). Trạng thái triển khai thực tế trên môi trường cloud là **"Đã bổ sung mã nguồn và hướng dẫn triển khai, cần người quản trị thiết lập secrets và thực hiện deploy lên dịch vụ Supabase của dự án"**.

---
*Người thực hiện: Nhóm phát triển phần mềm nhúng & Đám mây.*
