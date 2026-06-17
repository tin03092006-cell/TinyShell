#include <windows.h>

#include <iostream>
#include <string>
#include <vector>

#include "builtins/builtins.h"
#include "core/process_executor.h"
#include "core/process_manager.h"
#include "utils/command_parser.h"
#include "utils/console_io.h"
#include "utils/string_utils.h"

// Báo cáo các tiến trình nền đã hoàn thành
static void report_finished_processes() {
  for (const auto& [pid, exitCode] : remove_finished_processes()) {
    std::cout << "[Background process " << pid << " completed with exit code "
              << exitCode << "]\n";
  }
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

// Dọn dẹp tài nguyên và các tiến trình nền trước khi thoát shell một cách âm
// thầm
static void cleanup_and_exit() {
  // Thực hiện dọn dẹp các tiến trình mồ côi dưới nền nhưng không in log hay tạm
  // dừng, giống hệt như hành vi của Windows CMD khi gõ 'exit'.
  terminate_all_processes();
}

int main() {
  setup_environment();

  std::string input;
  while (true) {
    report_finished_processes();
    std::cout << get_prompt();

    InputState state = read_input_line(input);

    if (state == InputState::END_OF_FILE) break;
    if (state == InputState::INTERRUPTED) continue;
    if (input.empty()) continue;

    if (!process_command_input(input)) break;
  }

  cleanup_and_exit();
  return 0;
}