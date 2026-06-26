#ifndef CONSOLE_IO_H
#define CONSOLE_IO_H

#include <string>

// Trạng thái kết quả sau khi đọc một dòng lệnh
enum class InputState { SUCCESS, INTERRUPTED, END_OF_FILE };

// Cấu hình môi trường khởi tạo cho shell (xử lý ngắt, console handler...)
void setup_environment();

// Lấy chuỗi prompt hiển thị cho người dùng (chứa CWD)
std::string get_prompt();

// Đọc một dòng lệnh từ người dùng an toàn (hỗ trợ cả Console và Pipe/File)
InputState read_input_line(std::string& input);

// Kiểm tra xem người dùng có bấm Ctrl+C hay không, sau đó tự reset flag
bool check_and_reset_ctrl_c();

#endif  // CONSOLE_IO_H
