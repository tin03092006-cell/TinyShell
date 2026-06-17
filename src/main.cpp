#include <windows.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "builtins/builtins.h"
#include "core/process_executor.h"
#include "core/process_manager.h"
#include "utils/command_parser.h"
#include "utils/string_utils.h"

// Biến cờ (flag) để theo dõi trạng thái xem phím Ctrl+C đã được nhấn hay chưa
volatile bool ctrl_c_pressed = false;

// Hàm xử lý tín hiệu (signal handler) từ console, đặc biệt là sự kiện Ctrl+C
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
  if (!WriteConsoleInputA(GetStdHandle(STD_INPUT_HANDLE), ir, 4, &w)) {
    return FALSE;
  }
  return TRUE;
}

// Lấy chuỗi prompt hiển thị cho người dùng, chứa đường dẫn thư mục hiện tại
// (CWD)
static std::string get_prompt() {
  std::error_code ec;
  std::string cwd = std::filesystem::current_path(ec).string();
  return !ec ? cwd + "> " : "myShell> ";
}

// Phân tích và thực thi chuỗi lệnh nhập vào từ người dùng
// Trả về false nếu lệnh yêu cầu thoát shell, true nếu tiếp tục
static bool process_command_input(const std::string& input) {
  std::vector<std::string> args;
  try {
    args = parse_command(input);
  } catch (const std::invalid_argument& e) {
    std::cout << e.what() << "\n";
    return true;  // Continue running
  }

  bool is_background = detect_background(input, args);
  if (args.empty()) return true;

  std::string cmd = args[0];
  string_to_lower_inplace(cmd);

  if (BuiltinResult res = execute_builtin(cmd, args, is_background);
      res == BuiltinResult::EXIT_SHELL) {
    return false;  // Exit shell
  } else if (res == BuiltinResult::NOT_FOUND) {
    if (auto [success, bgPid] = execute_external(args, is_background);
        !success) {
      std::cout << "Error: Bad command or file name.\n";
    } else if (is_background && bgPid != 0) {
      std::cout << "[Background process started with PID " << bgPid << "]\n";
    }
  }
  return true;  // Continue running
}

int main() {
  std::cout << std::unitbuf;
  if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
    std::cerr << "Error: Failed to set control handler. Error: "
              << GetLastError() << "\n";
  }
  std::string input;

  while (true) {
    for (const auto& [pid, exitCode] : remove_finished_processes()) {
      std::cout << "[Background process " << pid << " completed with exit code "
                << exitCode << "]\n";
    }

    std::cout << get_prompt();
    if (!std::getline(std::cin, input)) {
      if (std::cin.eof()) break; // Người dùng ấn Ctrl+Z (EOF)
      std::cin.clear();          // Xóa trạng thái lỗi do ngắt tín hiệu (Ctrl+C)
    }
    if (ctrl_c_pressed) {
      ctrl_c_pressed = false;
      std::cout << "\n";
      continue;
    }
    if (input.empty()) continue;

    if (!process_command_input(input)) break;
  }

  std::cout << "Sending kill signal to all child background processes...\n";
  for (const auto& [pid, errorCode] : terminate_all_processes()) {
    std::cout << "Warning: Failed to terminate PID " << pid
              << " (Error: " << errorCode << ")\n";
  }

  return 0;
}