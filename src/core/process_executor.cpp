#include "process_executor.h"

#include <windows.h>

#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include "../utils/command_parser.h"
#include "process_manager.h"

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
      jeli.BasicLimitInformation.LimitFlags =
          JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
      if (!SetInformationJobObject(hJob, JobObjectExtendedLimitInformation,
                                   &jeli, sizeof(jeli))) {
        std::cerr << "Warning: Failed to set job object information. Error: "
                  << GetLastError() << "\n";
      }
    }
    if (!AssignProcessToJobObject(hJob, hProcess)) {
      std::cerr << "Warning: Failed to assign process to job object. Error: "
                << GetLastError() << "\n";
    }
  }
  return hJob;
}

// Hàm trợ giúp để chờ tiến trình foreground kết thúc
static void wait_foreground_process(HANDLE hProcess) {
  {
    std::lock_guard<std::mutex> lock(mtx_foreground);
    current_foreground_process = hProcess;
  }
  if (WaitForSingleObject(hProcess, INFINITE) == WAIT_FAILED) {
    std::cerr << "Error: Failed to wait for foreground process. Error: "
              << GetLastError() << "\n";
  }
  {
    std::lock_guard<std::mutex> lock(mtx_foreground);
    current_foreground_process = NULL;
  }
}

// Hàm thực thi các lệnh bên ngoài (không phải built-in)
// Quản lý việc nối chuỗi lệnh và tạo tiến trình mới (CreateProcess)
ExecutionResult execute_external(std::vector<std::string>& args,
                                 bool is_background) {
  std::string cmd_line = build_command_line(args);
  if (cmd_line.empty()) return {false, 0};

  STARTUPINFOA si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  DWORD flags =
      CREATE_SUSPENDED |
      (is_background ? CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW : 0);

  auto try_create = [&](std::string cmd) {
    return CreateProcessA(NULL, cmd.data(), NULL, NULL, FALSE, flags, NULL,
                          NULL, &si, &pi) != 0;
  };

  if (!try_create(cmd_line) &&
      !try_create("cmd.exe /s /c \"" + cmd_line + "\"")) {
    return {false, 0};
  }

  // Tạo Job Object cho cả foreground và background để quản lý toàn bộ process tree
  HANDLE hJob = create_and_assign_job(pi.hProcess, is_background);

  // Bây giờ process đã ở trong Job Object, ta mới resume cho nó chạy
  if (ResumeThread(pi.hThread) == (DWORD)-1) {
    std::cerr << "Warning: Failed to resume thread.\n";
  }
  if (!CloseHandle(pi.hThread)) {
    std::cerr << "Warning: Failed to close thread handle.\n";
  }

  if (is_background) {
    add_background_process(
        {pi.dwProcessId, cmd_line, pi.hProcess, hJob, ProcessState::RUNNING});
    return {true, pi.dwProcessId};
  } else {
    wait_foreground_process(pi.hProcess);
    if (hJob) {
      if (!CloseHandle(hJob))
        std::cerr << "Warning: Failed to close job handle.\n";
    }
    if (!CloseHandle(pi.hProcess)) {
      std::cerr << "Warning: Failed to close process handle.\n";
    }
    return {true, 0};
  }
}