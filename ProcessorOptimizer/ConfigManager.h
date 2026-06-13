#pragma once
#include <string>
#include <Windows.h>

namespace ConfigManager {
    // General Settings
    extern DWORD InitDelayMs;

    // System Settings
    extern bool DisableEcoQoS;
    extern bool EnableHighPrecisionTimer;

    // CPU Settings
    extern bool EnableAffinityFix;
    extern bool EnableSoftAffinity;
    extern DWORD_PTR CustomAffinityMask;

    // Process Settings
    extern int PriorityLevel;
    extern bool EnableWatchdog;

    // Loads configuration from the specified INI file path.
    void LoadConfig(const std::string& iniFilePath);
}