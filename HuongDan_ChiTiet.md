# Hướng Dẫn Chi Tiết Từng Lệnh Trong TinyShell

Tài liệu này giải thích cụ thể chức năng, cú pháp và cung cấp ví dụ cho từng lệnh được hỗ trợ trong TinyShell.

---

## 1. Nhóm Lệnh Hệ Thống Cơ Bản

### 1.1 Lệnh `help`
- **Chức năng:** Hiển thị bảng tổng hợp tất cả các lệnh mà shell đang hỗ trợ kèm mô tả ngắn gọn.
- **Cú pháp:** `help`
- **Ví dụ:** Gõ `help` và nhấn Enter.

### 1.2 Lệnh `cd` (Change Directory)
- **Chức năng:** Thay đổi thư mục làm việc hiện tại của shell.
- **Cú pháp:** `cd <đường_dẫn>`
- **Ví dụ:**
  - `cd ..` : Di chuyển ngược ra thư mục cha.
  - `cd src` : Di chuyển vào thư mục con tên là `src`.
  - `cd C:\Windows` : Di chuyển trực tiếp đến đường dẫn tuyệt đối `C:\Windows`.

### 1.3 Lệnh `dir`
- **Chức năng:** Liệt kê toàn bộ tệp tin (file) và thư mục con bên trong một thư mục.
- **Cú pháp:** `dir` (Thư mục hiện tại) hoặc `dir <đường_dẫn>` (Thư mục chỉ định)
- **Ví dụ:**
  - `dir` : Xem nội dung thư mục bạn đang đứng.
  - `dir C:\` : Xem nội dung của ổ đĩa C.

### 1.4 Lệnh `date` và `time`
- **Chức năng:** Xem ngày và giờ hiện tại của hệ thống máy tính.
- **Cú pháp:** `date` hoặc `time`

### 1.5 Lệnh `exit`
- **Chức năng:** Thoát khỏi chương trình TinyShell một cách an toàn. Tất cả các tiến trình chạy ngầm (nếu có) sẽ được dọn dẹp trước khi thoát để tránh rò rỉ bộ nhớ.
- **Cú pháp:** `exit`

---

## 2. Nhóm Lệnh Biến Môi Trường (Environment Variables)

### 2.1 Lệnh `path`
- **Chức năng:** In ra chuỗi đường dẫn hiện tại đang được lưu trong biến môi trường `PATH` của hệ điều hành. Nhờ biến `PATH`, hệ thống mới biết chỗ để tìm các file `.exe`.
- **Cú pháp:** `path`

### 2.2 Lệnh `addpath`
- **Chức năng:** Bổ sung thêm một thư mục mới vào biến môi trường `PATH` ngay trong phiên làm việc hiện tại.
- **Cú pháp:** `addpath <đường_dẫn>`
- **Ví dụ:**
  - `addpath C:\TestFolder` : Thêm `C:\TestFolder` vào biến PATH. Sau lệnh này, bạn có thể gọi trực tiếp bất kỳ file `.exe` nào nằm trong thư mục đó mà không cần gõ toàn bộ đường dẫn.

---

## 3. Nhóm Lệnh Quản Lý Tiến Trình (Process Management)

### 3.1 Chạy chương trình ngoại trú bình thường (Foreground)
- **Chức năng:** Gọi các chương trình khác trong máy tính (như notepad, ping, ipconfig...). Lệnh này sẽ chặn shell lại, bạn phải đợi chương trình đó chạy xong (hoặc đóng nó lại) thì mới gõ được lệnh tiếp theo.
- **Ví dụ:** 
  - `notepad.exe`
  - `ping google.com -n 5`
- **Lưu ý:** Nếu nhấn `Ctrl+C` lúc lệnh đang chạy, chương trình đó sẽ bị ngắt nhưng TinyShell vẫn an toàn không bị văng.

### 3.2 Toán tử `&` - Chạy ngầm (Background Process)
- **Chức năng:** Khi bạn gõ dấu `&` ở cuối bất kỳ lệnh nào, chương trình đó sẽ được khởi chạy ngầm. TinyShell sẽ trả lại ngay dấu nhắc lệnh để bạn làm việc khác.
- **Cú pháp:** `<tên_chương_trình> &`
- **Ví dụ:** `notepad.exe &` (Notepad sẽ bật lên nhưng bạn vẫn có thể gõ lệnh khác trong shell ngay lập tức).

### 3.3 Lệnh `list`
- **Chức năng:** Liệt kê danh sách các chương trình mà bạn đã cho chạy ngầm bằng toán tử `&`. Bảng liệt kê sẽ hiển thị ID, mã số tiến trình hệ thống (PID), trạng thái (Đang chạy / Bị dừng) và Tên lệnh.
- **Cú pháp:** `list`

### 3.4 Lệnh `stop` (Tạm dừng)
- **Chức năng:** Tạm dừng (đóng băng) hoạt động của một tiến trình đang chạy ngầm. Tiến trình đó sẽ không thể tiêu thụ CPU hay nhận tương tác cho đến khi được mở lại.
- **Cú pháp:** `stop <pid>`
- **Ví dụ:** `stop 1234` (Đóng băng tiến trình có mã PID là 1234).

### 3.5 Lệnh `resume` (Tiếp tục)
- **Chức năng:** Phục hồi (mở đóng băng) một tiến trình đã bị lệnh `stop` tạm dừng.
- **Cú pháp:** `resume <pid>`
- **Ví dụ:** `resume 1234`

### 3.6 Lệnh `kill` (Kết liễu)
- **Chức năng:** Ép buộc một tiến trình ngầm phải đóng lại ngay lập tức. Tính năng này được TinyShell quản lý rất chặt (thông qua Windows Job Objects), nên nếu tiến trình đó có đẻ ra các tiến trình con thì tất cả cũng sẽ bị kết liễu sạch sẽ.
- **Cú pháp:** `kill <pid>`
- **Ví dụ:** `kill 1234`

---

## Kịch Bản Chạy Thử Thực Tế

Hãy gõ lần lượt các dòng sau để hiểu rõ cơ chế chạy ngầm:

1. Chạy ngầm 2 chương trình:
   `notepad.exe &`
   `calc.exe &`

2. Xem mã PID của chúng:
   `list`
   *(Giả sử notepad có PID là 1234)*

3. Thử đóng băng notepad:
   `stop 1234`
   *(Bạn có thể thử gõ chữ vào cửa sổ notepad, sẽ thấy nó bị đơ)*

4. Cho notepad chạy lại:
   `resume 1234`

5. Tắt notepad đi:
   `kill 1234`
