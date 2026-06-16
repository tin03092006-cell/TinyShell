#ifndef BUILTINS_H
#define BUILTINS_H
#include <string>
#include <vector>

// Hiển thị danh sách các lệnh hỗ trợ và cách sử dụng cơ bản
void execute_help();

// Hiển thị ngày hiện tại
void execute_date();

// Hiển thị giờ hiện tại
void execute_time();

// Liệt kê các tập tin và thư mục (hỗ trợ wildcard hoặc đường dẫn cụ thể)
void execute_dir(const std::vector<std::string>& args);

// Thay đổi thư mục hiện hành
void execute_cd(const std::vector<std::string>& args);

// Hiển thị biến môi trường PATH hiện tại
void execute_path();

// Thêm một thư mục mới vào biến môi trường PATH
void execute_addpath(const std::string& newPath);

// Liệt kê trạng thái của tất cả tiến trình ngầm hiện tại
void execute_list();

// Kết thúc một tiến trình ngầm dựa trên PID
bool execute_kill(const std::vector<std::string>& args);

// Tạm dừng (suspend) một tiến trình ngầm
bool execute_stop(const std::vector<std::string>& args);

// Tiếp tục (resume) một tiến trình ngầm đã bị tạm dừng
bool execute_resume(const std::vector<std::string>& args);

// Kết quả trả về của hàm thực thi lệnh builtin
enum class BuiltinResult {
  NOT_FOUND,  // Lệnh không phải là builtin
  EXECUTED,   // Lệnh là builtin và đã thực thi xong
  EXIT_SHELL  // Lệnh là 'exit', yêu cầu thoát shell
};

// Kiểm tra và thực thi lệnh builtin nếu có
BuiltinResult execute_builtin(const std::string& cmd,
                              const std::vector<std::string>& args,
                              bool is_background);

#endif  // BUILTINS_H
