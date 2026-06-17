#include <iostream>
#include <string>
#include <windows.h>

volatile bool ctrl_c = false;
BOOL WINAPI Handler(DWORD type) {
    if (type == CTRL_C_EVENT) {
        ctrl_c = true;
        return TRUE; // handled
    }
    return FALSE;
}

int main() {
    SetConsoleCtrlHandler(Handler, TRUE);
    std::string s;
    std::cout << "Type something (Ctrl+C to interrupt): ";
    if (!std::getline(std::cin, s)) {
        std::cout << "\ngetline failed. eof: " << std::cin.eof() << " fail: " << std::cin.fail() << " ctrl_c: " << ctrl_c << "\n";
    } else {
        std::cout << "\ngetline succeeded. s: " << s << " ctrl_c: " << ctrl_c << "\n";
    }
    return 0;
}
