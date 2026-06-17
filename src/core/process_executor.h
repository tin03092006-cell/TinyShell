#ifndef PROCESS_EXECUTOR_H
#define PROCESS_EXECUTOR_H

#include <windows.h>

#include <string>
#include <vector>

// Cấu trúc lưu trữ kết quả của việc thực thi tiến trình
struct ExecutionResult {
  bool success;         // Thành công hay không
  DWORD backgroundPid;  // PID của tiến trình ngầm (nếu có), hoặc 0 nếu chạy nền
                        // thất bại/chạy foreground
};

// Kiểm tra xem có tiến trình foreground nào đang chạy hay không
bool has_foreground_process();

// Thực thi một chương trình ngoại trú
// args: danh sách tham số, phần tử đầu tiên là tên chương trình
// is_background: cờ xác định chạy nền (background) hay chạy foreground
ExecutionResult execute_external(std::vector<std::string>& args,
                                 bool is_background);

#endif  // PROCESS_EXECUTOR_H
