#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H
#include <windows.h>

#include <string>
#include <vector>

#include "process_info.h"

// Cấu trúc lưu trữ thông tin tiến trình ngầm đã hoàn thành
struct FinishedProcess {
  DWORD pid;       // PID của tiến trình
  DWORD exitCode;  // Mã thoát (exit code) của tiến trình
};

// Cấu trúc lưu trữ thông tin lỗi khi kết thúc tiến trình
struct TerminateError {
  DWORD pid;        // PID của tiến trình lỗi
  DWORD errorCode;  // Mã lỗi từ hệ điều hành
};

// Enum đại diện cho các trạng thái kết quả khi thao tác với tiến trình
enum class ProcessActionStatus {
  SUCCESS,           // Thao tác thành công
  NOT_FOUND,         // Không tìm thấy tiến trình (hoặc không quản lý)
  ALREADY_EXITED,    // Tiến trình đã kết thúc từ trước
  ALREADY_IN_STATE,  // Tiến trình đã ở trạng thái yêu cầu (vd: đã stop)
  FAILED,            // Thao tác thất bại do lỗi hệ thống
  FAILED_DETACHED    // Thất bại vì tiến trình bị tách rời (detached)
};

// Thêm một tiến trình vào danh sách quản lý ngầm
void add_background_process(const ProcessInfo& p);

// Lấy số lượng tiến trình ngầm đang quản lý
size_t get_background_process_count();

// Lấy danh sách toàn bộ tiến trình ngầm (kèm theo trạng thái hiện tại)
std::vector<ProcessInfo> api_list_processes();

// Dọn dẹp các tiến trình ngầm đã hoàn thành
std::vector<FinishedProcess> remove_finished_processes();

// Buộc kết thúc tất cả các tiến trình ngầm trước khi thoát shell
std::vector<TerminateError> terminate_all_processes();

// Kết thúc một tiến trình ngầm dựa trên PID
ProcessActionStatus api_kill_process(DWORD pid);

// Tạm dừng (suspend) một tiến trình ngầm
ProcessActionStatus api_suspend_process(DWORD pid);

// Tiếp tục (resume) một tiến trình ngầm đã bị tạm dừng
ProcessActionStatus api_resume_process(DWORD pid);

#endif  // PROCESS_MANAGER_H
