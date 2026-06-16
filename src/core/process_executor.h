#ifndef PROCESS_EXECUTOR_H
#define PROCESS_EXECUTOR_H

#include <string>
#include <vector>
#include <windows.h>

struct ExecutionResult {
  bool success;
  DWORD backgroundPid;
};

bool has_foreground_process();
ExecutionResult execute_external(std::vector<std::string>& args, bool is_background);

#endif  // PROCESS_EXECUTOR_H
