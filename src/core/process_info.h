#ifndef PROCESS_INFO_H
#define PROCESS_INFO_H
#include <windows.h>

#include <string>

// Cấu trúc lưu trữ thông tin của một tiến trình ngầm
struct ProcessInfo {
  DWORD pid;            // Process ID do Windows cấp
  std::string command;  // Tên lệnh hoặc command line đã chạy
  HANDLE hProcess;  // Handle của tiến trình để quản lý (wait, kill, suspend)
  HANDLE hJob;      // Job Object handle để quản lý toàn bộ process tree
  bool isRunning;   // Trạng thái: đang chạy hay đang bị tạm dừng (suspended)
  bool isFinished;  // Trạng thái: đã kết thúc hay chưa
};

#endif  // PROCESS_INFO_H
