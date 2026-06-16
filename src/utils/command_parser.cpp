#include "command_parser.h"

#include <cctype>
#include <stdexcept>

// Hàm phân tách chuỗi đầu vào thành các Token (tham số)
// Xử lý giữ nguyên khoảng trắng nếu nằm trong dấu ngoặc kép ""
// Và tự động tách các toán tử đặc biệt như & thành token riêng
std::vector<std::string> parse_command(const std::string& input) {
  std::vector<std::string> args; std::string arg;
  bool in_quotes = false, has_token = false;
  for (size_t i = 0; i < input.size(); ++i) {
    char c = input[i];
    if (c == '\\' && i + 1 < input.size()) {
      char next = input[++i];
      arg += (next == '"' || next == '\\') ? next : (--i, c);
      has_token = true;
    } else if (c == '"') { in_quotes = !in_quotes; has_token = true; }
    else if (!in_quotes && (isspace(static_cast<unsigned char>(c)) || c == '&')) {
      if (has_token || !arg.empty()) { args.push_back(arg); arg.clear(); has_token = false; }
      if (c == '&') args.push_back("&");
    } else { arg += c; has_token = true; }
  }
  if (in_quotes) { throw std::invalid_argument("Error: Unmatched quote in command."); }
  if (has_token || !arg.empty()) args.push_back(arg);
  return args;
}

// Kiểm tra xem lệnh có kết thúc bằng dấu '&' (yêu cầu chạy ngầm) hay không
// Nếu có, xóa '&' khỏi danh sách tham số và trả về true.
bool detect_background(const std::string& input, std::vector<std::string>& args) {
  if (args.empty() || args.back() != "&") return false;
  size_t pos = input.find_last_not_of(" \t\n\r");
  if (pos == std::string::npos || input[pos] != '&') return false;
  
  size_t bs = 0;
  for (int i = static_cast<int>(pos) - 1; i >= 0 && input[i] == '\\'; --i) bs++;
  if (bs % 2 != 0) return false;

  args.pop_back();
  return true;
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

// Hàm phụ trợ: Escape một tham số dòng lệnh
static std::string escape_argument(const std::string& arg) {
  if (arg.empty() || arg.find_first_of(" \t\n&|^<>\"") != std::string::npos) {
    std::string escaped = "\"";
    for (size_t j = 0; j <= arg.size(); ++j) {
      size_t bs = 0;
      while (j < arg.size() && arg[j] == '\\') { ++bs; ++j; }
      if (j == arg.size()) { escaped.append(bs * 2, '\\'); break; }
      else if (arg[j] == '"') { escaped.append(bs * 2 + 1, '\\'); escaped += '"'; }
      else { escaped.append(bs, '\\'); escaped += arg[j]; }
    }
    escaped += '"';
    return escaped;
  }
  return arg;
}

// Hàm nối các tham số thành một chuỗi dòng lệnh hoàn chỉnh (có xử lý ngoặc kép và escape)
std::string build_command_line(const std::vector<std::string>& args) {
  std::string cmd_line;
  for (size_t i = 0; i < args.size(); ++i) {
    cmd_line += escape_argument(args[i]);
    if (i < args.size() - 1) cmd_line += " ";
  }
  return cmd_line;
}
