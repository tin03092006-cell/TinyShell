#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H
#include <windows.h>
#include <string>
#include <vector>
#include "process_info.h"

struct FinishedProcess {
  DWORD pid;
  DWORD exitCode;
};

struct TerminateError {
  DWORD pid;
  DWORD errorCode;
};

enum class ProcessActionStatus {
  SUCCESS,
  NOT_FOUND,
  ALREADY_EXITED,
  ALREADY_IN_STATE,
  FAILED,
  FAILED_DETACHED
};

void add_background_process(const ProcessInfo& p);
size_t get_background_process_count();
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
