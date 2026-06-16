#include "command_parser.h"

#include <cctype>
#include <iostream>

// Hàm phân tách chuỗi đầu vào thành các Token (tham số)
// Xử lý giữ nguyên khoảng trắng nếu nằm trong dấu ngoặc kép ""
// Và tự động tách các toán tử đặc biệt như & thành token riêng
std::vector<std::string> parse_command(const std::string& input) {
  std::vector<std::string> args; std::string arg;
  bool in_quotes = false, has_token = false;
  for (size_t i = 0; i < input.size(); ++i) {
    char c = input[i];
    if (c == '\\' && !in_quotes && i + 1 < input.size()) {
      char next = input[++i];
      arg += (next == '"' || next == '\\') ? next : (--i, c);
      has_token = true;
    } else if (c == '"') { in_quotes = !in_quotes; has_token = true; }
    else if (!in_quotes && (isspace(static_cast<unsigned char>(c)) || c == '&')) {
      if (has_token || !arg.empty()) { args.push_back(arg); arg.clear(); has_token = false; }
      if (c == '&') args.push_back("&");
    } else { arg += c; has_token = true; }
  }
  if (in_quotes) { std::cout << "Error: Unmatched quote in command.\n"; return {}; }
  if (has_token || !arg.empty()) args.push_back(arg);
  return args;
}

// Kiểm tra xem lệnh có kết thúc bằng dấu '&' (yêu cầu chạy ngầm) hay không
// Nếu có, xóa '&' khỏi danh sách tham số và trả về true.
bool detect_background(std::vector<std::string>& args) {
  if (!args.empty() && args.back() == "&") {
    args.pop_back();
    return true;
  }
  return false;
}

// Hàm nối các phần tử của mảng thành một chuỗi duy nhất, cách nhau bằng khoảng
// trắng
std::string join_args(const std::vector<std::string>& args, size_t start) {
  std::string result;
  for (size_t i = start; i < args.size(); ++i) {
    if (i > start) result += " ";
    result += args[i];
  }
  return result;
}

// Hàm nối các tham số thành một chuỗi dòng lệnh hoàn chỉnh (có xử lý ngoặc kép và escape)
std::string build_command_line(const std::vector<std::string>& args) {
  std::string cmd_line;
  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i].empty() || args[i].find_first_of(" &|^<>\"") != std::string::npos) {
      cmd_line += '"';
      for (size_t j = 0; j <= args[i].size(); ++j) {
        size_t bs = 0;
        while (j < args[i].size() && args[i][j] == '\\') { ++bs; ++j; }
        if (j == args[i].size()) { cmd_line.append(bs * 2, '\\'); break; }
        else if (args[i][j] == '"') { cmd_line.append(bs * 2 + 1, '\\'); cmd_line += '"'; }
        else { cmd_line.append(bs, '\\'); cmd_line += args[i][j]; }
      }
      cmd_line += '"';
    } else cmd_line += args[i];
    if (i < args.size() - 1) cmd_line += " ";
  }
  return cmd_line;
}
