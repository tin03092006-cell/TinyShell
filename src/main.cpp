#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "builtins/builtins.h"
#include "core/process_executor.h"
#include "core/process_manager.h"
#include "utils/command_parser.h"
#include "utils/string_utils.h"

volatile bool ctrl_c_pressed = false;

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
  if (fdwCtrlType == CTRL_BREAK_EVENT) return TRUE;
  if (fdwCtrlType != CTRL_C_EVENT) return FALSE;
  if (has_foreground_process()) return TRUE;
  
  ctrl_c_pressed = true;

  INPUT_RECORD ir[4] = {};
  for (int i = 0; i < 4; i++) {
    ir[i].EventType = KEY_EVENT;
    ir[i].Event.KeyEvent = {
        static_cast<BOOL>(!(i % 2)),
        1,
        (WORD)(i < 2 ? VK_F24 : VK_RETURN),
        (WORD)(i > 1 ? MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC) : 0),
        {(WCHAR)(i > 1 ? '\r' : 0)},
        0};
  }
  DWORD w;
  WriteConsoleInputA(GetStdHandle(STD_INPUT_HANDLE), ir, 4, &w);
  return TRUE;
}

static std::string get_prompt() {
  std::error_code ec;
  std::string cwd = std::filesystem::current_path(ec).string();
  return !ec ? cwd + "> " : "myShell> ";
}

int main() {
  std::cout << std::unitbuf;
  SetConsoleCtrlHandler(CtrlHandler, TRUE);
  std::string input;

  while (true) {
    auto finished = remove_finished_processes();
    for (const auto& f : finished) {
      std::cout << "[Background process " << f.pid
                << " completed with exit code " << f.exitCode << "]\n";
    }

    std::cout << get_prompt();
    if (!std::getline(std::cin, input)) break;
    if (ctrl_c_pressed) {
      ctrl_c_pressed = false;
      std::cout << "\n";
      continue;
    }
    if (input.empty()) continue;

    std::vector<std::string> args;
    try {
      args = parse_command(input);
    } catch (const std::invalid_argument& e) {
      std::cout << e.what() << "\n";
      continue;
    }
    
    bool is_background = detect_background(input, args);
    if (args.empty()) continue;

    std::string cmd = args[0];
    string_to_lower_inplace(cmd);

    int res = execute_builtin(cmd, args, is_background);
    if (res == 1) break;
    if (res == -1) {
      ExecutionResult execRes = execute_external(args, is_background);
      if (!execRes.success) {
        std::cout << "Error: Bad command or file name.\n";
      } else if (is_background && execRes.backgroundPid != 0) {
        std::cout << "[Background process started with PID " << execRes.backgroundPid << "]\n";
      }
    }
  }

  std::cout << "Sending kill signal to all child background processes...\n";
  auto errors = terminate_all_processes();
  for (const auto& err : errors) {
    std::cout << "Warning: Failed to terminate PID " << err.pid
              << " (Error: " << err.errorCode << ")\n";
  }

  return 0;
}