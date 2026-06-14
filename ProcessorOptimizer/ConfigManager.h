#pragma once
#include <string>
#include <Windows.h>

namespace ConfigManager {
    // General Settings
    extern DWORD InitDelayMs;
    extern bool EnableLogging;

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

    // Memory Settings (New)
    extern bool EnableMemoryOptimization;
    extern bool EnableSmartTrim;
    extern DWORD_PTR BloatThresholdMB;
    extern DWORD QuietPeriodSeconds;
    extern bool EnableLFH;

    // Loads configuration from the specified INI file path.
    void LoadConfig(const std::string& iniFilePath);
}