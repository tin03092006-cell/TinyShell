#include "builtins.h"

#include <windows.h>

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "../core/process_manager.h"
#include "../utils/command_parser.h"
#include "../utils/string_utils.h"

// --- UTILITY FUNCTIONS ---
static bool handle_pid_cmd(const std::vector<std::string>& args,
                           ProcessActionStatus (*func)(DWORD),
                           const char* action_verb, const char* already_state,
                           const char* name) {
  if (args.size() < 2) {
    std::cout << "Usage: " << name << " <pid>\n";
    return true;
  }
  if (!args[1].empty() &&
      args[1].find_first_not_of("0123456789") == std::string::npos) {
    try {
      DWORD pid = std::stoul(args[1]);
      ProcessActionStatus status = func(pid);
      std::string pstr = std::to_string(pid);
      switch (status) {
        case ProcessActionStatus::SUCCESS:
          std::cout << "Process " << pstr << " " << action_verb << ".\n";
          break;
        case ProcessActionStatus::NOT_FOUND:
          std::cout << "Error: Process " << pstr << " not found.\n";
          break;
        case ProcessActionStatus::ALREADY_EXITED:
          std::cout << "Process " << pstr
                    << " has already exited. Removing from list.\n";
          break;
        case ProcessActionStatus::ALREADY_IN_STATE:
          std::cout << "Process " << pstr << " is already " << already_state
                    << ".\n";
          break;
        case ProcessActionStatus::FAILED_DETACHED:
          std::cout << "Failed to " << name << " process " << pstr
                    << ". Process tree is detached.\n";
          break;
        case ProcessActionStatus::FAILED:
        default:
          std::cout << "Failed to " << name << " process " << pstr << ".\n";
          break;
      }
    } catch (const std::out_of_range&) {
      std::cout << "Error: Invalid PID (out of range).\n";
    }
  } else {
    std::cout << "Error: Invalid PID.\n";
  }
  return true;
}

// --- SYSTEM/INFO COMMANDS ---
void execute_help() {
  std::cout << R"(
  WELCOME TO MY SHELL

myShell supports the following commands:
  dir      : List the contents of the current directory (or specific path)
  list     : List all background processes
  kill     : Kill a background process by PID (e.g., kill 1234)
  stop     : Suspend a background process by PID (e.g., stop 1234)
  resume   : Resume a background process by PID (e.g., resume 1234)
  path     : Display current PATH
  addpath  : Add a directory to PATH (e.g., addpath C:\test)
  date     : Show current date
  time     : Show current time
  exit     : Exit my shell
  <cmd>    : Execute the program given by <cmd>
  <cmd> &  : Execute the program in background

)";
}

void execute_date() {
  SYSTEMTIME st;
  GetLocalTime(&st);
  printf("Current date: %02d/%02d/%d\n", st.wDay, st.wMonth, st.wYear);
}

void execute_time() {
  SYSTEMTIME st;
  GetLocalTime(&st);
  printf("Current time: %02d:%02d:%02d\n", st.wHour, st.wMinute, st.wSecond);
}

// --- FILESYSTEM COMMANDS ---
void execute_cd(const std::vector<std::string>& args) {
  try {
    if (args.size() > 1)
      std::filesystem::current_path(join_args(args));
    else
      std::cout << std::filesystem::current_path().string() << "\n";
  } catch (const std::filesystem::filesystem_error&) {
    std::cout << "Error: The system cannot find the path specified.\n";
  }
}

void execute_dir(const std::vector<std::string>& args) {
  std::string target = (args.size() > 1) ? join_args(args) : ".";
  std::string search_pattern = target;

  std::error_code ec;
  if (std::filesystem::is_directory(target, ec)) {
    search_pattern += "\\*";
  }

  WIN32_FIND_DATAA findData;
  HANDLE hFind = FindFirstFileA(search_pattern.c_str(), &findData);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      std::string filename = findData.cFileName;
      if (filename != "." && filename != "..") {
        std::cout << filename << "\n";
      }
    } while (FindNextFileA(hFind, &findData));
    if (!FindClose(hFind)) {
      std::cerr << "Warning: Failed to close find handle.\n";
    }
  } else {
    std::cout << "Error: File not found.\n";
  }
}

// --- ENVIRONMENT COMMANDS ---
void execute_path() {
  char buffer[32767];
  DWORD length = GetEnvironmentVariableA("PATH", buffer, sizeof(buffer));
  std::cout << (length > 0 && length < sizeof(buffer)
                    ? "PATH=" + std::string(buffer) + "\n"
                : length >= sizeof(buffer)
                    ? "Error: PATH too long to display.\n"
                    : "PATH is not set.\n");
}

void execute_addpath(const std::string& newPath) {
  std::error_code ec;
  if (!std::filesystem::is_directory(newPath, ec)) {
    std::cout << "Warning: Directory does not exist: " << newPath << "\n";
    return;
  }
  char buffer[32767];
  DWORD length = GetEnvironmentVariableA("PATH", buffer, sizeof(buffer));
  std::string currentPath =
      (length > 0 && length < sizeof(buffer)) ? buffer : "";

  std::string curLower = ";" + currentPath + ";",
              newLower = ";" + newPath + ";";
  string_to_lower_inplace(curLower);
  string_to_lower_inplace(newLower);
  if (curLower.find(newLower) != std::string::npos) {
    std::cout << "Warning: '" << newPath << "' is already in PATH.\n";
    return;
  }

  std::string updatedPath =
      currentPath.empty() ? newPath : (currentPath + ";" + newPath);
  if (updatedPath.size() > 32767) {
    std::cout
        << "Error: PATH would exceed maximum length (32767 characters).\n";
    return;
  }
  if (!SetEnvironmentVariableA("PATH", updatedPath.c_str())) {
    std::cout << "Error: Failed to update PATH. Error: " << GetLastError()
              << "\n";
  } else {
    std::cout << "PATH updated.\n";
  }
}

// --- PROCESS MANAGEMENT COMMANDS ---
void execute_list() {
  auto processes = api_list_processes();
  if (processes.empty()) {
    std::cout << "No background processes running.\n";
    return;
  }
  std::cout << std::left << std::setw(5) << "ID" << std::setw(12) << "PID"
            << std::setw(12) << "Status" << "Command\n";
  int id = 1;
  for (const auto& p : processes) {
    std::string statusStr = (p.state == ProcessState::FINISHED)  ? "Exited"
                            : (p.state == ProcessState::RUNNING) ? "Running"
                                                                 : "Stopped";
    std::cout << std::left << std::setw(5) << id++ << std::setw(12) << p.pid
              << std::setw(12) << statusStr << p.command << "\n";
  }
}

bool execute_kill(const std::vector<std::string>& args) {
  return handle_pid_cmd(args, api_kill_process, "killed", "killed", "kill");
}

bool execute_stop(const std::vector<std::string>& args) {
  return handle_pid_cmd(args, api_suspend_process, "suspended", "suspended",
                        "stop");
}

bool execute_resume(const std::vector<std::string>& args) {
  return handle_pid_cmd(args, api_resume_process, "resumed", "running",
                        "resume");
}

BuiltinResult execute_builtin(const std::string& cmd,
                              const std::vector<std::string>& args,
                              bool is_bg) {
  if (cmd == "exit") {
    remove_finished_processes();
    if (size_t c = get_background_process_count(); c > 0)
      std::cout << "Warning: " << c
                << " background process(es) still running. They will be "
                   "terminated.\n";
    return BuiltinResult::EXIT_SHELL;
  }

  struct Cmd {
    const char* name;
    void (*func)(const std::vector<std::string>&);
  };
  static const Cmd builtins[] = {
      {"cd", execute_cd},
      {"help", [](const auto&) { execute_help(); }},
      {"date", [](const auto&) { execute_date(); }},
      {"time", [](const auto&) { execute_time(); }},
      {"dir", execute_dir},
      {"path", [](const auto&) { execute_path(); }},
      {"addpath",
       [](const auto& a) {
         if (a.size() > 1)
           execute_addpath(join_args(a));
         else
           std::cout << "Usage: addpath <directory>\n";
       }},
      {"list", [](const auto&) { execute_list(); }},
      {"kill", [](const auto& a) { execute_kill(a); }},
      {"stop", [](const auto& a) { execute_stop(a); }},
      {"resume", [](const auto& a) { execute_resume(a); }}};

  for (const auto& b : builtins) {
    if (cmd == b.name) {
      if (is_bg)
        std::cout << "Warning: '" << cmd << "' cannot run in background.\n";
      b.func(args);
      return BuiltinResult::EXECUTED;
    }
  }

  return BuiltinResult::NOT_FOUND;
}
