#ifndef PROCESS_INFO_H
#define PROCESS_INFO_H
#include <windows.h>

#include <string>

// Định nghĩa các trạng thái của vòng đời một tiến trình ngầm
enum class ProcessState {
  RUNNING,   // Đang thực thi bình thường
  SUSPENDED  // Bị tạm dừng (bởi lệnh stop)
};

// Cấu trúc lưu trữ thông tin của một tiến trình ngầm
struct ProcessInfo {
  DWORD pid;            // Process ID do Windows cấp
  std::string command;  // Tên lệnh hoặc command line đã chạy
  HANDLE hProcess;     // Handle của tiến trình để quản lý (wait, kill, suspend)
  HANDLE hJob;         // Job Object handle để quản lý toàn bộ process tree
  ProcessState state;  // Trạng thái hiện tại của tiến trình
};

#endif  // PROCESS_INFO_H
