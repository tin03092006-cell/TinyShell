#include <windows.h>

#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "builtins/builtins.h"
#include "core/process_executor.h"
#include "core/process_manager.h"
#include "utils/command_parser.h"
#include "utils/console_io.h"
#include "utils/string_utils.h"

static void cleanup_and_exit();
static bool process_command_input(const std::string& input);

static void execute_bat_file(const std::string& bat_file, const std::vector<std::string>& args) {
  std::ifstream file(bat_file);
  if (!file.is_open()) return;
  std::string line;
  while (std::getline(file, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line.empty()) continue;
    
    std::string trimmed_line = line;
    trimmed_line.erase(0, trimmed_line.find_first_not_of(" \t"));
    if (trimmed_line.empty()) continue;

    if (trimmed_line[0] == ':') continue;

    std::string lower_line = trimmed_line;
    string_to_lower_inplace(lower_line);
    
    if (lower_line.length() >= 5 && lower_line.substr(0, 5) == "goto ") {
      std::string label = trimmed_line.substr(5);
      label.erase(0, label.find_first_not_of(" \t"));
      size_t endpos = label.find_last_not_of(" \t");
      if (std::string::npos != endpos) label = label.substr(0, endpos + 1);
      
      label = ":" + label;
      string_to_lower_inplace(label);

      file.clear();
      file.seekg(0, std::ios::beg);
      std::string search_line;
      while (std::getline(file, search_line)) {
        if (!search_line.empty() && search_line.back() == '\r') search_line.pop_back();
        search_line.erase(0, search_line.find_first_not_of(" \t"));
        if (search_line.empty()) continue;
        
        std::string search_lower = search_line;
        string_to_lower_inplace(search_lower);
        size_t sl_end = search_lower.find_last_not_of(" \t");
        if (std::string::npos != sl_end) search_lower = search_lower.substr(0, sl_end + 1);
        
        if (search_lower == label) break;
      }
      continue;
    }

    for (size_t i = 1; i <= 9 && i < args.size(); ++i) {
      std::string arg_ph = "%" + std::to_string(i);
      size_t pos = 0;
      while ((pos = line.find(arg_ph, pos)) != std::string::npos) {
        line.replace(pos, arg_ph.length(), args[i]);
        pos += args[i].length();
      }
    }

    if (!process_command_input(line)) {
      cleanup_and_exit();
      exit(0); 
    }

    // Nếu người dùng bấm Ctrl+C trong khi tiến trình đang chạy, dừng việc đọc batch file
    if (check_and_reset_ctrl_c()) {
      std::cout << "^C\n";
      break;
    }
  }
}


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
    std::string bat_file;
    if (cmd.length() >= 4 && cmd.substr(cmd.length() - 4) == ".bat" && std::filesystem::is_regular_file(cmd)) {
      bat_file = cmd;
    } else if (std::filesystem::is_regular_file(cmd + ".bat")) {
      bat_file = cmd + ".bat";
    }

    if (!bat_file.empty()) {
      if (is_background) {
        std::thread(execute_bat_file, bat_file, args).detach();
        std::cout << "[Background process started for BAT file]\n";
      } else {
        execute_bat_file(bat_file, args);
      }
      return true;
    }

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