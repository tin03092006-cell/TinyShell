#include "process_manager.h"

#include <windows.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

static std::vector<ProcessInfo> background_processes;
static std::mutex mtx_background;

void add_background_process(const ProcessInfo& p) {
  std::lock_guard<std::mutex> lock(mtx_background);
  background_processes.push_back(p);
}
size_t get_background_process_count() {
  std::lock_guard<std::mutex> lock(mtx_background);
  return background_processes.size();
}

// Hàm tiện ích đóng Handle của một ProcessInfo
static void close_handles(ProcessInfo& p) {
  if (p.hProcess) {
    CloseHandle(p.hProcess);
    p.hProcess = NULL;
  }
  if (p.hJob) {
    CloseHandle(p.hJob);
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
// Hàm này quét qua danh sách background_processes, dùng WaitForSingleObject để
// kiểm tra trạng thái. Nếu tiến trình đã kết thúc, nó sẽ tính toán thời gian
// chạy, ghi log và giải phóng Handle. C3: Thiết kế 2-pass có chủ đích: lần gọi
// đầu đánh dấu isFinished=true (để execute_list hiển thị "Exited"),
//     lần gọi sau mới xóa entry khỏi vector. Đảm bảo user luôn thấy thông báo
//     kết thúc ít nhất 1 lần.
void remove_finished_processes() {
  std::lock_guard<std::mutex> lock(mtx_background);
  for (auto& p : background_processes) {
    if (p.isFinished) continue;
    DWORD wr = WaitForSingleObject(p.hProcess, 0);
    if ((wr != WAIT_OBJECT_0 && wr != WAIT_FAILED) ||
        (wr == WAIT_OBJECT_0 && job_still_active(p.hJob)))
      continue;
    if (wr == WAIT_OBJECT_0) {
      DWORD code = 0;
      GetExitCodeProcess(p.hProcess, &code);
      std::cout << "[Background process " << p.pid
                << " completed with exit code " << code << "]\n";
    }
    close_handles(p);
    p.isRunning = false;
    p.isFinished = true;
  }
}

// Hàm dọn dẹp tất cả các tiến trình ngầm khi thoát shell
// Gọi TerminateProcess/TerminateJobObject để ép kết thúc các tiến trình đang
// chạy ngầm và đóng Handle.
void terminate_all_processes() {
  std::lock_guard<std::mutex> lock(mtx_background);
  for (auto& p : background_processes) {
    if (!p.isFinished && WaitForSingleObject(p.hProcess, 0) != WAIT_OBJECT_0)
      if (!(p.hJob ? TerminateJobObject(p.hJob, 0)
                   : TerminateProcess(p.hProcess, 0)))
        std::cout << "Warning: Failed to terminate PID " << p.pid
                  << " (Error: " << GetLastError() << ")\n";
    close_handles(p);
  }
  background_processes.clear();
}

// Dọn dẹp entry rác (những tiến trình đã báo cáo Exited)
static void clean_exited_processes() {
  background_processes.erase(
      std::remove_if(background_processes.begin(), background_processes.end(),
                     [](const ProcessInfo& p) { return p.isFinished; }),
      background_processes.end());
}

std::string api_list_processes() {
  std::lock_guard<std::mutex> lock(mtx_background);
  if (background_processes.empty()) return "No background processes running.\n";
  std::ostringstream oss;
  oss << std::left << std::setw(5) << "ID" << std::setw(12) << "PID"
      << std::setw(12) << "Status" << "Command\n";
  int id = 1;
  for (const auto& p : background_processes)
    oss << std::left << std::setw(5) << id++ << std::setw(12) << p.pid
        << std::setw(12)
        << (p.isFinished ? "Exited" : (p.isRunning ? "Running" : "Stopped"))
        << p.command << "\n";
  clean_exited_processes();
  return oss.str();
}

// Lệnh kill: Kết thúc một tiến trình ngầm theo PID
// Dùng TerminateProcess/TerminateJobObject để ngắt vòng đời của tiến trình
std::string api_kill_process(DWORD pid) {
  std::lock_guard<std::mutex> lock(mtx_background);
  auto it = std::find_if(background_processes.begin(), background_processes.end(),
                         [pid](const auto& p) { return p.pid == pid; });
  if (it == background_processes.end())
    return "Error: Process " + std::to_string(pid) + " not found.\n";

  std::string pstr = std::to_string(pid), res;
  if (is_dead(*it)) res = "Process " + pstr + " has already exited. Removing from list.\n";
  else res = (it->hJob ? TerminateJobObject(it->hJob, 0) : TerminateProcess(it->hProcess, 0))
                 ? "Process " + pstr + " killed.\n"
                 : "Failed to kill process " + pstr + ".\n";

  close_handles(*it);
  background_processes.erase(it);
  return res;
}

// Hàm trợ giúp để suspend/resume tất cả tiến trình trong một Job Object
static bool toggle_job_processes(HANDLE hJob, LONG(NTAPI* func)(HANDLE)) {
  DWORD cb = sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) + 256 * sizeof(ULONG_PTR);
  std::vector<BYTE> buf(cb);
  auto pList = reinterpret_cast<PJOBOBJECT_BASIC_PROCESS_ID_LIST>(buf.data());
  
  if (!QueryInformationJobObject(hJob, JobObjectBasicProcessIdList, pList, cb, &cb)) {
    buf.resize(cb); pList = reinterpret_cast<PJOBOBJECT_BASIC_PROCESS_ID_LIST>(buf.data());
    if (!QueryInformationJobObject(hJob, JobObjectBasicProcessIdList, pList, cb, &cb)) return false;
  }
  
  for (DWORD i = 0; i < pList->NumberOfProcessIdsInList; ++i) {
    if (HANDLE h = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, static_cast<DWORD>(pList->ProcessIdList[i]))) {
      func(h); CloseHandle(h);
    }
  }
  return true;
}

static std::string toggle_process(DWORD pid, bool suspend) {
  std::lock_guard<std::mutex> lock(mtx_background);
  auto it = std::find_if(background_processes.begin(), background_processes.end(), [pid](const auto& p) { return p.pid == pid; });
  std::string pstr = std::to_string(pid), act = suspend ? "suspend" : "resume";

  if (it == background_processes.end()) return "Error: Process " + pstr + " not found in background list.\n";
  if (is_dead(*it)) {
    close_handles(*it); background_processes.erase(it);
    return "Process " + pstr + " has already exited. Removing from list.\n";
  }
  if (it->isRunning == !suspend) return "Process " + pstr + " is already " + (suspend ? "suspended.\n" : "running.\n");

  auto func = (LONG(NTAPI*)(HANDLE))GetProcAddress(GetModuleHandleA("ntdll.dll"), suspend ? "NtSuspendProcess" : "NtResumeProcess");
  if (!func) return "Error: Cannot find function in ntdll.dll\n";

  bool success = false;
  if (it->hJob) {
    success = toggle_job_processes(it->hJob, func);
  } else if (WaitForSingleObject(it->hProcess, 0) != WAIT_OBJECT_0) {
    LONG s = func(it->hProcess);
    if (s < 0) return "Failed to " + act + " process " + pstr + ". NTSTATUS: " + std::to_string(s) + "\n";
    success = true;
  }

  if (!success) return "Failed to " + act + " process " + pstr + ". Process tree is detached.\n";
  it->isRunning = !suspend;
  return "Process " + pstr + " " + act + "ed.\n";
}

// Lệnh stop: Tạm dừng một tiến trình ngầm theo PID
std::string api_suspend_process(DWORD pid) { return toggle_process(pid, true); }

// Lệnh resume: Tiếp tục chạy một tiến trình ngầm đã bị suspend
std::string api_resume_process(DWORD pid) { return toggle_process(pid, false); }
