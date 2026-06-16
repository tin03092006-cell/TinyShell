#include "process_manager.h"

#include <windows.h>

#include <algorithm>
#include <iostream>
#include <mutex>
#include <vector>

// Danh sách lưu trữ các tiến trình ngầm đang được shell quản lý
static std::vector<ProcessInfo> background_processes;
// Mutex để bảo đảm an toàn luồng (thread-safety) khi truy cập danh sách tiến trình ngầm
static std::mutex mtx_background;

void add_background_process(const ProcessInfo& p) {
  std::lock_guard<std::mutex> lock(mtx_background);
  background_processes.push_back(p);
}
size_t get_background_process_count() {
  std::lock_guard<std::mutex> lock(mtx_background);
  return std::count_if(background_processes.begin(), background_processes.end(),
                       [](const ProcessInfo& p) { return !p.isFinished; });
}

// Hàm tiện ích đóng Handle của một ProcessInfo
static void close_handles(ProcessInfo& p) {
  if (p.hProcess) {
    if (!CloseHandle(p.hProcess)) {
      std::cerr << "Warning: Failed to close process handle.\n";
    }
    p.hProcess = NULL;
  }
  if (p.hJob) {
    if (!CloseHandle(p.hJob)) {
      std::cerr << "Warning: Failed to close job handle.\n";
    }
    p.hJob = NULL;
  }
}

// Kiểm tra xem Job Object còn tiến trình con đang chạy hay không.
// Trên Windows hiện đại, một số app (vd notepad) là process shim: process gốc
// thoát nhanh nhưng spawn child thật vẫn nằm trong Job. Nếu đóng hJob lúc này,
// JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE sẽ kill child → app bị tắt bất ngờ.
static bool job_still_active(HANDLE hJob) {
  if (!hJob) return false;
  JOBOBJECT_BASIC_ACCOUNTING_INFORMATION info{};
  return QueryInformationJobObject(hJob, JobObjectBasicAccountingInformation,
                                   &info, sizeof(info), NULL) &&
         info.ActiveProcesses > 0;
}

// Kiểm tra tiến trình đã thực sự kết thúc chưa (dùng chung cho kill/toggle)
static bool is_dead(const ProcessInfo& p) {
  if (p.isFinished) return true;
  bool rootExited = WaitForSingleObject(p.hProcess, 0) == WAIT_OBJECT_0;
  return rootExited && !job_still_active(p.hJob);
}

// Hàm dọn dẹp các tiến trình ngầm (zombie processes) đã hoàn thành
// Trả về danh sách các tiến trình vừa hoàn thành cùng với mã lỗi (exit code)
std::vector<FinishedProcess> remove_finished_processes() {
  std::vector<FinishedProcess> finished;
  std::lock_guard<std::mutex> lock(mtx_background);
  auto it = background_processes.begin();
  while (it != background_processes.end()) {
    if (it->isFinished) {
      it = background_processes.erase(it);
      continue;
    }
    DWORD wr = WaitForSingleObject(it->hProcess, 0);
    if ((wr != WAIT_OBJECT_0 && wr != WAIT_FAILED) ||
        (wr == WAIT_OBJECT_0 && job_still_active(it->hJob))) {
      ++it;
      continue;
    }
    if (wr == WAIT_OBJECT_0) {
      DWORD code = 0;
      if (!GetExitCodeProcess(it->hProcess, &code)) {
        std::cerr << "Warning: Failed to get exit code.\n";
      }
      finished.push_back({it->pid, code});
    }
    close_handles(*it);
    it->isRunning = false;
    it->isFinished = true;
    ++it;
  }
  return finished;
}

// Hàm dọn dẹp tất cả các tiến trình ngầm khi thoát shell
// Trả về danh sách các lỗi terminate nếu có
std::vector<TerminateError> terminate_all_processes() {
  std::vector<TerminateError> errors;
  std::lock_guard<std::mutex> lock(mtx_background);
  for (auto& p : background_processes) {
    if (!p.isFinished && WaitForSingleObject(p.hProcess, 0) != WAIT_OBJECT_0)
      if (!(p.hJob ? TerminateJobObject(p.hJob, 0)
                   : TerminateProcess(p.hProcess, 0)))
        errors.push_back({p.pid, GetLastError()});
    close_handles(p);
  }
  background_processes.clear();
  return errors;
}

// Dọn dẹp entry rác (những tiến trình đã báo cáo Exited)
static void clean_exited_processes() {
  background_processes.erase(
      std::remove_if(background_processes.begin(), background_processes.end(),
                     [](const ProcessInfo& p) { return p.isFinished; }),
      background_processes.end());
}

std::vector<ProcessInfo> api_list_processes() {
  std::lock_guard<std::mutex> lock(mtx_background);
  std::vector<ProcessInfo> result = background_processes;
  clean_exited_processes();
  return result;
}

// Lệnh kill: Kết thúc một tiến trình ngầm theo PID
ProcessActionStatus api_kill_process(DWORD pid) {
  std::lock_guard<std::mutex> lock(mtx_background);
  auto it =
      std::find_if(background_processes.begin(), background_processes.end(),
                   [pid](const auto& p) { return p.pid == pid; });
  if (it == background_processes.end()) return ProcessActionStatus::NOT_FOUND;

  if (is_dead(*it)) {
    close_handles(*it);
    background_processes.erase(it);
    return ProcessActionStatus::ALREADY_EXITED;
  }

  bool success = (it->hJob ? TerminateJobObject(it->hJob, 0)
                           : TerminateProcess(it->hProcess, 0));
  close_handles(*it);
  background_processes.erase(it);
  return success ? ProcessActionStatus::SUCCESS : ProcessActionStatus::FAILED;
}

// Hàm trợ giúp để suspend/resume tất cả tiến trình trong một Job Object
static bool toggle_job_processes(HANDLE hJob, LONG(NTAPI* func)(HANDLE)) {
  DWORD cb = sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) + 256 * sizeof(ULONG_PTR);
  std::vector<BYTE> buf(cb);
  auto pList = reinterpret_cast<PJOBOBJECT_BASIC_PROCESS_ID_LIST>(buf.data());

  if (!QueryInformationJobObject(hJob, JobObjectBasicProcessIdList, pList, cb,
                                 &cb)) {
    if (GetLastError() == ERROR_MORE_DATA) {
      cb = static_cast<DWORD>(sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) +
                              pList->NumberOfProcessIdsInList *
                                  sizeof(ULONG_PTR));
      buf.resize(cb);
      pList = reinterpret_cast<PJOBOBJECT_BASIC_PROCESS_ID_LIST>(buf.data());
      if (!QueryInformationJobObject(hJob, JobObjectBasicProcessIdList, pList,
                                     cb, &cb))
        return false;
    } else {
      return false;
    }
  }

  for (DWORD i = 0; i < pList->NumberOfProcessIdsInList; ++i) {
    if (HANDLE h = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE,
                               static_cast<DWORD>(pList->ProcessIdList[i]))) {
      func(h);
      CloseHandle(h);
    }
  }
  return true;
}

// Hàm cốt lõi để thực hiện tạm dừng (suspend) hoặc tiếp tục (resume) một tiến trình ngầm
static ProcessActionStatus toggle_process(DWORD pid, bool suspend) {
  std::lock_guard<std::mutex> lock(mtx_background);
  auto it =
      std::find_if(background_processes.begin(), background_processes.end(),
                   [pid](const auto& p) { return p.pid == pid; });

  if (it == background_processes.end()) return ProcessActionStatus::NOT_FOUND;
  if (is_dead(*it)) {
    close_handles(*it);
    background_processes.erase(it);
    return ProcessActionStatus::ALREADY_EXITED;
  }
  if (it->isRunning == !suspend) return ProcessActionStatus::ALREADY_IN_STATE;

  auto farProc =
      GetProcAddress(GetModuleHandleA("ntdll.dll"),
                     suspend ? "NtSuspendProcess" : "NtResumeProcess");
  if (!farProc) return ProcessActionStatus::FAILED;
  // Cast through void* to suppress -Wcast-function-type warning in GCC
  auto func =
      reinterpret_cast<LONG(NTAPI*)(HANDLE)>(reinterpret_cast<void*>(farProc));

  bool success = false;
  if (it->hJob) {
    success = toggle_job_processes(it->hJob, func);
  } else if (WaitForSingleObject(it->hProcess, 0) != WAIT_OBJECT_0) {
    LONG s = func(it->hProcess);
    if (s < 0) return ProcessActionStatus::FAILED;
    success = true;
  }

  if (!success) return ProcessActionStatus::FAILED_DETACHED;
  it->isRunning = !suspend;
  return ProcessActionStatus::SUCCESS;
}

// Lệnh stop: Tạm dừng một tiến trình ngầm theo PID
ProcessActionStatus api_suspend_process(DWORD pid) {
  return toggle_process(pid, true);
}

// Lệnh resume: Tiếp tục chạy một tiến trình ngầm đã bị suspend
ProcessActionStatus api_resume_process(DWORD pid) {
  return toggle_process(pid, false);
}
