#include "string_utils.h"

#include <algorithm>
#include <cctype>

// Chuyển đổi toàn bộ các ký tự trong chuỗi thành chữ thường một cách an toàn
void string_to_lower_inplace(std::string& str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return std::tolower(c); });
}
