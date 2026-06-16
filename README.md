# TinyShellOS

Một dự án shell đơn giản chạy trên hệ điều hành Windows được viết bằng C++.

## Yêu cầu hệ thống (Prerequisites)

Để biên dịch và chạy dự án này trên máy tính của bạn (Windows), bạn cần cài đặt các công cụ sau:

1. **Trình biên dịch C++ (g++) hỗ trợ chuẩn C++17 trở lên**:
   - Khuyến nghị sử dụng **MinGW-w64** hoặc **MSYS2** trên Windows.
   - Bạn có thể tải MSYS2 tại trang chủ: [https://www.msys2.org/](https://www.msys2.org/)
   - Sau khi cài đặt MSYS2, mở terminal **MSYS2 UCRT64** và chạy lệnh cài đặt toolchain gcc:
     ```bash
     pacman -S mingw-w64-ucrt-x86_64-gcc
     ```
   - **Quan trọng**: Hãy chắc chắn bạn đã thêm đường dẫn tới thư mục `bin` chứa `g++.exe` (ví dụ: `C:\msys64\ucrt64\bin`) vào biến môi trường **PATH** của Windows.

2. **Git** (để clone mã nguồn):
   - Tải và cài đặt Git tại: [https://git-scm.com/](https://git-scm.com/)

## Hướng dẫn cài đặt và chạy

1. **Clone repository (tải mã nguồn về máy)**:
   Mở terminal (Command Prompt, PowerShell hoặc Git Bash) và chạy lệnh:
   ```bash
   git clone https://github.com/tin03092006-cell/TinyShell.git
   cd TinyShell
   ```

2. **Biên dịch mã nguồn**:
   Dự án đã có sẵn kịch bản (script) để biên dịch tự động. Bạn chỉ cần chạy file `build.bat` nằm trong thư mục `scripts`:
   ```bash
   .\scripts\build.bat
   ```
   Nếu quá trình biên dịch diễn ra thành công, bạn sẽ thấy thông báo `Compilation successful.` và một file thực thi mang tên `myshell.exe` sẽ được tạo ra tại thư mục gốc của dự án.

3. **Chạy Shell**:
   Khởi chạy shell bằng lệnh:
   ```bash
   .\myshell.exe
   ```
   Lúc này, bạn có thể bắt đầu gõ các lệnh trong shell của mình!

## Cấu trúc thư mục

- `src/`: Chứa mã nguồn C++ của dự án.
  - `builtins/`: Các lệnh tích hợp sẵn (cd, dir, help...).
  - `core/`: Quản lý tiến trình (Foreground, Background, Job Objects).
  - `utils/`: Xử lý phân tích dòng lệnh (Parser) và chuỗi.
- `scripts/`: Chứa các tệp script hỗ trợ (ví dụ: `build.bat` để biên dịch dự án).
- `.clang-format`: Cấu hình chuẩn định dạng code (formatting) cho toàn bộ dự án.

## Tính năng chính (Features)

Các chức năng được hỗ trợ:
- **Lệnh hệ thống cơ bản**: `dir`, `cd`, `date`, `time`, `help`, `exit`.
- **Biến môi trường**: Xem biến môi trường với lệnh `path` và thêm đường dẫn bằng `addpath`.
- **Thực thi chương trình ngoại trú**: Chạy ứng dụng `.exe` hoặc file batch.
- **Chạy nền (Background Processing)**: 
  - Thêm toán tử `&` ở cuối lệnh để chạy ngầm.
  - Quản lý tiến trình ngầm: `list` (xem danh sách), `kill <pid>` (kết thúc), `stop <pid>` (tạm dừng), `resume <pid>` (tiếp tục).

## Kỹ thuật triển khai (Technical Details)

Dự án sử dụng các **Windows API** sau để quản lý tài nguyên:
- **Job Objects (`CreateJobObject`)**: Nhóm process tree của một ứng dụng chạy nền. Đảm bảo khi lệnh `kill` được gọi, ứng dụng gốc và các ứng dụng con do nó sinh ra đều bị kết thúc, ngăn chặn rò rỉ bộ nhớ (Memory Leak) và tiến trình thây ma (Zombie Process).
- **Native API (`NtSuspendProcess` / `NtResumeProcess`)**: Tải từ `ntdll.dll` để suspend/resume toàn bộ tiến trình trong Job Object.
- **Thread-safe**: Dùng `std::mutex` để bảo vệ tài nguyên chia sẻ khi shell kiểm tra trạng thái tiến trình nền.
- **Console Signal Handling**: Dùng `SetConsoleCtrlHandler` để ngắt ứng dụng đang chạy ở foreground khi người dùng nhấn `Ctrl+C` mà không làm thoát shell.

## Ví dụ sử dụng (Examples)

```bash
myShell> dir
myShell> cd C:\Windows
myShell> notepad.exe &
[Background process started with PID 1234]
myShell> list
ID   PID         Status      Command
1    1234        Running     notepad.exe
myShell> stop 1234
Process 1234 suspended.
myShell> resume 1234
Process 1234 resumed.
myShell> kill 1234
Process 1234 killed.
```
