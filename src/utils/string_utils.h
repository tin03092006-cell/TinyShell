#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <string>

// Chuyển đổi toàn bộ các ký tự trong chuỗi thành chữ thường một cách an toàn (tránh Undefined Behavior với non-ASCII)
void string_to_lower_inplace(std::string& str);

#endif // STRING_UTILS_H
