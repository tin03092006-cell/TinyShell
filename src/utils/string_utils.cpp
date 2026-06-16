#include "string_utils.h"
#include <algorithm>
#include <cctype>

void string_to_lower_inplace(std::string& str) {
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
}
