@echo off
cd %~dp0..
echo Compiling TinyShellOS...
g++ -std=c++17 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -g -Isrc -o myshell.exe src/utils/command_parser.cpp src/utils/string_utils.cpp src/builtins/builtins.cpp src/core/process_manager.cpp src/core/process_executor.cpp src/main.cpp
if %errorlevel% neq 0 (
    echo Compilation failed!
    exit /b %errorlevel%
)
echo Compilation successful. Run myshell.exe to start.
