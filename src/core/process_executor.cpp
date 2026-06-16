#include "process_executor.h"

#include <windows.h>

#include <string>
#include <vector>

#include "process_manager.h"
#include <mutex>
#include "../utils/command_parser.h"

static HANDLE current_foreground_process = NULL;
static std::mutex mtx_foreground;

bool has_foreground_process() {
  std::lock_guard<std::mutex> lock(mtx_foreground);
  return current_foreground_process != NULL;
}

// Hàm trợ giúp để tạo Job Object và gán tiến trình
static HANDLE create_and_assign_job(HANDLE hProcess, bool is_background) {
  HANDLE hJob = CreateJobObject(NULL, NULL);
  if (hJob) {
    if (is_background) {
      JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli{};
      jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
      SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
    }
    AssignProcessToJobObject(hJob, hProcess);
  }
  return hJob;
}

// Hàm trợ giúp để chờ tiến trình foreground kết thúc
static void wait_foreground_process(HANDLE hProcess, HANDLE hJob) {
  {
    std::lock_guard<std::mutex> lock(mtx_foreground);
    current_foreground_process = hProcess;
  }
  WaitForSingleObject(hProcess, INFINITE);
  {
    std::lock_guard<std::mutex> lock(mtx_foreground);
    current_foreground_process = NULL;
  }
}

// Hàm thực thi các lệnh bên ngoài (không phải built-in)
// Quản lý việc nối chuỗi lệnh và tạo tiến trình mới (CreateProcess)
ExecutionResult execute_external(std::vector<std::string>& args, bool is_background) {
  std::string cmd_line = build_command_line(args);
  if (cmd_line.empty()) return {false, 0};

  STARTUPINFOA si{sizeof(si)};
  PROCESS_INFORMATION pi{};
  DWORD flags = CREATE_SUSPENDED | (is_background ? CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW : 0);

  auto try_create = [&](std::string cmd) {
    return CreateProcessA(NULL, cmd.data(), NULL, NULL, FALSE, flags, NULL, NULL, &si, &pi) != 0;
  };

  if (!try_create(cmd_line) &&
      !try_create("cmd.exe /s /c \"" + cmd_line + "\"")) {
    return {false, 0};
  }

  // M4: Tạo Job Object cho CẢ foreground và background để quản lý toàn bộ process tree
  HANDLE hJob = create_and_assign_job(pi.hProcess, is_background);

  // Bây giờ process đã ở trong Job Object, ta mới resume cho nó chạy
  ResumeThread(pi.hThread);
  CloseHandle(pi.hThread);

  if (is_background) {
    add_background_process({pi.dwProcessId, cmd_line, pi.hProcess, hJob, true, false});
    return {true, pi.dwProcessId};
  } else {
    wait_foreground_process(pi.hProcess, hJob);
    if (hJob) CloseHandle(hJob);
    CloseHandle(pi.hProcess);
    return {true, 0};
  }
}