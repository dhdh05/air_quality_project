# Thư mục chứa hình ảnh báo cáo (LaTeX Figures Assets)

Thư mục này lưu trữ toàn bộ các tệp tin hình ảnh đồ họa sử dụng trong tài liệu LaTeX của báo cáo.

## Danh sách các ảnh đã được tạo sẵn trong dự án:

Tất cả các ảnh cần thiết cho tiến trình biên dịch báo cáo đã được tôi sinh tự động và lưu trữ sẵn trong thư mục này:
1. `logo_hust.png`: Ảnh logo/biểu trưng đại học thiết kế vector tinh giản đặt ở bìa báo cáo (`main.tex`).
2. `wiring_detail.png`: Sơ đồ kết nối vật lý logic giữa vi điều khiển ESP32 và các cảm biến.
3. `prototype_overview.png`: Ảnh dựng 3D mô phỏng mô hình nguyên mẫu trạm đo cục bộ tích hợp quạt hút trong hộp acrylic.
4. `figures_correlation_matrix.png`: Ma trận hệ số tương quan các biến đo đạc (PM2.5, Nhiệt độ, Độ ẩm, Áp suất, v.v.).
5. `figures_error_comparison.png`: Biểu đồ so sánh sai số MAE và RMSE của các thuật toán hiệu chuẩn.
6. `figures_feature_importance.png`: Đánh giá mức độ quan trọng đặc trưng đóng góp trong mô hình Random Forest.
7. `figures_predictions_comparison.png`: Đồ thị phân tán kết quả dự đoán của Linear Regression và Random Forest trên tập Test.
8. `figures_scatter_raw_vs_ref.png`: Đồ thị phân tán so sánh nồng độ bụi thô của cảm biến với trạm tham chiếu quốc gia.

## Hướng dẫn cập nhật hình ảnh:
- Nếu bạn có ảnh chụp sản phẩm thật hoặc sơ đồ đi dây Altium chính xác của nhóm, bạn chỉ cần thay thế các tệp tin tương ứng (`logo_hust.png`, `wiring_detail.png`, `prototype_overview.png`) trong thư mục này bằng ảnh mới của bạn. Tên tệp tin phải được giữ nguyên để LaTeX tự động cập nhật khi Recompile.
