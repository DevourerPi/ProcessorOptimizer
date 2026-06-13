#pragma once
#include <Windows.h>
#include <string>

namespace Utils {
    // Retrieves the directory path where the DLL is currently located.
    std::string GetCurrentDirectoryPath(HMODULE hModule);

    // Checks if the current process is running with Administrator privileges.
    bool IsProcessRunAsAdmin();
}