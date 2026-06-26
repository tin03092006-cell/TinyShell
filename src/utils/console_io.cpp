#include "console_io.h"

#include <windows.h>

#include <atomic>
#include <csignal>
#include <filesystem>
#include <iostream>

#include "../core/process_executor.h"

// Dùng atomic để tránh race condition giữa handler thread và main thread
static std::atomic<bool> ctrl_c_pressed{false};

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
  if (fdwCtrlType == CTRL_BREAK_EVENT) return TRUE;
  if (fdwCtrlType != CTRL_C_EVENT) return FALSE;

  ctrl_c_pressed.store(true, std::memory_order_seq_cst);

  // Nếu đang có foreground process, để Windows tự gửi Ctrl+C cho nó
  if (has_foreground_process()) return TRUE;

  return TRUE;
}

bool check_and_reset_ctrl_c() {
  return ctrl_c_pressed.exchange(false, std::memory_order_seq_cst);
}

void setup_environment() {
  // Bỏ qua SIGINT mặc định của C-Runtime
  signal(SIGINT, SIG_IGN);
  std::cout << std::unitbuf;

  if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
    std::cerr << "Error: Failed to set control handler. Error: "
              << GetLastError() << "\n";
  }
}

std::string get_prompt() {
  std::error_code ec;
  std::string cwd = std::filesystem::current_path(ec).string();
  return !ec ? cwd + "> " : "myShell> ";
}

InputState read_input_line(std::string& input) {
  HANDLE hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
  bool read_success = false;
  bool is_eof = false;
  input.clear();

  DWORD mode;
  if (GetConsoleMode(hConsoleInput, &mode)) {
    char buf[1024];
    DWORD read_bytes = 0;
    if (ReadConsoleA(hConsoleInput, buf, sizeof(buf), &read_bytes, NULL)) {
      if (read_bytes == 0) {
        // Khi gõ ở Console, cả Ctrl+C và Ctrl+Z đều có thể trả về 0 bytes.
        is_eof = false;
      } else {
        for (DWORD i = 0; i < read_bytes; ++i) {
          if (buf[i] == '\r' || buf[i] == '\n') break;
          input += buf[i];
        }
        read_success = true;
      }
    } else {
      if (GetLastError() == ERROR_OPERATION_ABORTED) {
        read_success = false;
      } else {
        is_eof = true;
      }
    }
  } else {
    // Dùng ReadFile để tránh bug cache EOF của UCRT (std::cin) khi đọc từ Pipe
    char ch;
    DWORD read_bytes = 0;
    read_success = true;
    while (true) {
      if (ReadFile(hConsoleInput, &ch, 1, &read_bytes, NULL)) {
        if (read_bytes == 0) {  // EOF
          if (input.empty()) {
            read_success = false;
            is_eof = true;
          }
          break;
        }
        if (ch == '\r') continue;
        if (ch == '\n') break;
        input += ch;
      } else {
        read_success = false;
        if (GetLastError() == ERROR_OPERATION_ABORTED) {
          // Bị ngắt bởi Ctrl+C
        } else {
          // Gộp chung các lỗi khác (bao gồm ERROR_BROKEN_PIPE) thành is_eof
          is_eof = true;
        }
        break;
      }
    }
  }

  bool was_ctrl_c = ctrl_c_pressed.exchange(false);

  if (!read_success) {
    if (was_ctrl_c || !is_eof) {
      std::cout << "\n";
      return InputState::INTERRUPTED;
    }
    return InputState::END_OF_FILE;
  }

  return InputState::SUCCESS;
}
