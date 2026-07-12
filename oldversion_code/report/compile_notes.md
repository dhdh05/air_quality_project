# Hướng Dẫn Biên Dịch Báo Cáo LaTeX (LaTeX Compilation Guide)

Tài liệu này hướng dẫn cách biên dịch file mã nguồn báo cáo `report/main.tex` thành tệp tin PDF hoàn chỉnh.

## 1. Biên Dịch Trên Overleaf (Khuyên Dùng)

Overleaf là nền tảng soạn thảo LaTeX trực tuyến miễn phí và nhanh chóng, không yêu cầu cài đặt phần mềm dưới local.
1. Truy cập vào trang web [Overleaf](https://www.overleaf.com/) và đăng nhập tài khoản.
2. Tạo một dự án mới: chọn **New Project** $\rightarrow$ **Blank Project** và đặt tên dự án (ví dụ: `Bao_Cao_ET3300`).
3. Upload toàn bộ cấu trúc thư mục `report/` trong repo này lên Overleaf (bao gồm thư mục `chapters/`, `appendices/`, `images/`, `tables/`, file `main.tex` và `references.bib`).
4. Tại thanh công cụ Menu của Overleaf (ở góc trên bên trái):
   - **Compiler**: Chọn **pdfLaTeX** (Mặc định).
   - **TeX Live Version**: Chọn phiên bản mới nhất (ví dụ: **2023** hoặc **2024**).
   - **Main document**: Chọn **main.tex**.
5. Nhấn nút **Recompile** (hoặc tổ hợp phím `Ctrl + Enter`) để biên dịch dự án. Dự án sẽ biên dịch thành công 100% ra file PDF ở màn hình bên phải.

## 2. Biên Dịch Local (Máy Tính Cá Nhân)

Nếu bạn muốn biên dịch trực tiếp trên máy tính cá nhân của mình:
1. Yêu cầu cài đặt bộ phân phối LaTeX đầy đủ:
   - **Windows**: Cài đặt [MiKTeX](https://miktex.org/) hoặc [TeX Live](https://www.tug.org/texlive/).
   - **macOS**: Cài đặt [MacTeX](https://www.tug.org/mactex/).
   - **Linux**: Cài đặt gói `texlive-full` qua package manager.
2. Cài đặt một trình soạn thảo LaTeX tiện dụng như [TeXstudio](https://www.texstudio.org/) hoặc extension LaTeX Workshop trên VS Code.
3. Mở file `report/main.tex` bằng TeXstudio.
4. Nhấn nút **Build & View** (Phím tắt `F5` trong TeXstudio) để tự động chạy tiến trình biên dịch qua pdfLaTeX và xuất ra file `main.pdf`.

## 3. Các Lỗi Thường Gặp Và Cách Sửa

- **Lỗi thiếu file ảnh**: Nếu bạn gặp cảnh báo `File ... not found`, đừng lo lắng. Cơ chế macro `\insertimage` định nghĩa trong dự án sẽ tự động phát hiện và chuyển sang vẽ khung chữ nhật thế chỗ (placeholder frame), không làm tiến trình biên dịch bị crash. Bạn chỉ cần upload ảnh thật vào thư mục `images/` là ảnh sẽ tự động hiển thị đè lên khung.
- **Lỗi hiển thị tiếng Việt**: Nếu tiếng Việt bị lỗi font dấu (ví dụ: chữ có dấu hỏi, ngã bị mất hoặc biến dạng), hãy kiểm tra chắc chắn trình biên dịch đang được thiết lập là **pdfLaTeX** và file `main.tex` có dòng khai báo `\usepackage[utf8]{vietnam}` ở đầu.
- **Lỗi trích dẫn tài liệu tham khảo (Bibliography)**: Nếu danh sách tài liệu tham khảo ở cuối trang báo cáo trống hoặc hiển thị các dấu chấm hỏi `[?]` tại vị trí citation:
  - Hãy chạy biên dịch theo đúng thứ tự 4 bước sau: 
    1. `pdflatex main`
    2. `bibtex main`
    3. `pdflatex main`
    4. `pdflatex main`
  - Trên Overleaf, trình biên dịch trực tuyến sẽ tự động xử lý chu kỳ này cho bạn khi nhấn **Recompile**.
