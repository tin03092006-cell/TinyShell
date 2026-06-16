#ifndef PROCESS_EXECUTOR_H
#define PROCESS_EXECUTOR_H

#include <string>
#include <vector>

bool has_foreground_process();

// Thực thi một lệnh ngoại trú (external command) ở foreground hoặc background
void execute_external(std::vector<std::string>& args, bool is_background);

#endif  // PROCESS_EXECUTOR_H
