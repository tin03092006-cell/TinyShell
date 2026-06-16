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


BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
  if (fdwCtrlType == CTRL_BREAK_EVENT) return TRUE;
  if (fdwCtrlType != CTRL_C_EVENT) return FALSE;
  if (has_foreground_process()) return TRUE;

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
    remove_finished_processes();
    std::cout << get_prompt();
    if (!std::getline(std::cin, input)) break;
    if (input.empty()) continue;

    std::vector<std::string> args = parse_command(input);
    bool is_background = detect_background(args);
    if (args.empty()) continue;

    std::string cmd = args[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    int res = execute_builtin(cmd, args, is_background);
    if (res == 1) break;
    if (res == -1) execute_external(args, is_background);
  }

  std::cout << "Sending kill signal to all child background processes...\n";
  terminate_all_processes();

  return 0;
}