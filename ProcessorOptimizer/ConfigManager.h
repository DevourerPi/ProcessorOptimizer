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
    extern int EnableAffinityFix; // int: 0=Off, 1=Only Core 0, 2=All Cores
    extern bool EnableSoftAffinity;
    extern DWORD_PTR CustomAffinityMask;

    // Process Settings
    extern int PriorityLevel;
    extern bool EnableWatchdog;

    // Loads configuration from the specified INI file path.
    void LoadConfig(const std::string& iniFilePath);
}