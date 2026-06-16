#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H
#include <string>
#include <vector>

// Phân tách chuỗi dòng lệnh thành các tham số (token), xử lý ngoặc kép và các
// toán tử đặc biệt
std::vector<std::string> parse_command(const std::string& input);

// Kiểm tra xem lệnh có yêu cầu chạy ngầm (chứa dấu '&' ở cuối) hay không
bool detect_background(std::vector<std::string>& args);

// Nối mảng tham số thành một chuỗi duy nhất cách nhau bởi khoảng trắng (bắt đầu
// từ vị trí start)
std::string join_args(const std::vector<std::string>& args, size_t start = 1);

// Nối mảng tham số thành một chuỗi dòng lệnh hoàn chỉnh có xử lý ngoặc kép và
// escape
std::string build_command_line(const std::vector<std::string>& args);

#endif  // COMMAND_PARSER_H