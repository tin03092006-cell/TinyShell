#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H
#include <windows.h>
#include <string>
#include "process_info.h"

void add_background_process(const ProcessInfo& p);
size_t get_background_process_count();
std::string api_list_processes();

// Dọn dẹp các tiến trình ngầm đã hoàn thành và lưu log
void remove_finished_processes();

// Buộc kết thúc tất cả các tiến trình ngầm trước khi thoát shell
void terminate_all_processes();

// Kết thúc một tiến trình ngầm dựa trên PID và trả về thông báo kết quả
std::string api_kill_process(DWORD pid);

// Tạm dừng (suspend) một tiến trình ngầm và trả về thông báo kết quả
std::string api_suspend_process(DWORD pid);

// Tiếp tục (resume) một tiến trình ngầm đã bị tạm dừng và trả về thông báo kết
// quả
std::string api_resume_process(DWORD pid);

#endif  // PROCESS_MANAGER_H
