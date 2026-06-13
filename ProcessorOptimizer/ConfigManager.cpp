#include "ConfigManager.h"
#include "Logger.h"

namespace ConfigManager {
    // Default values
    DWORD InitDelayMs = 0;
    bool EnableLogging = false;

    bool DisableEcoQoS = false;
    bool EnableHighPrecisionTimer = false;

    int EnableAffinityFix = 0;
    bool EnableSoftAffinity = false;
    DWORD_PTR CustomAffinityMask = 0;

    int PriorityLevel = 0;
    bool EnableWatchdog = false;

    void LoadConfig(const std::string& iniFilePath) {
        // General
        EnableLogging = GetPrivateProfileIntA("General", "EnableLogging", 1, iniFilePath.c_str()) != 0;
        InitDelayMs = GetPrivateProfileIntA("General", "InitDelayMs", 8000, iniFilePath.c_str());

        Logger::Log("ConfigManager: Loading configuration...");

        // System
        DisableEcoQoS = GetPrivateProfileIntA("System", "DisableEcoQoS", 1, iniFilePath.c_str()) != 0;
        EnableHighPrecisionTimer = GetPrivateProfileIntA("System", "EnableHighPrecisionTimer", 1, iniFilePath.c_str()) != 0;

        // CPU
        EnableAffinityFix = GetPrivateProfileIntA("CPU", "EnableAffinityFix", 1, iniFilePath.c_str());
        EnableSoftAffinity = GetPrivateProfileIntA("CPU", "EnableSoftAffinity", 1, iniFilePath.c_str()) != 0;

        // Process
        PriorityLevel = GetPrivateProfileIntA("Process", "PriorityLevel", 0, iniFilePath.c_str());
        EnableWatchdog = GetPrivateProfileIntA("Process", "EnableWatchdog", 1, iniFilePath.c_str()) != 0;

        // Parse Hex String for Custom Affinity Mask
        char maskStr[64] = { 0 };
        GetPrivateProfileStringA("CPU", "CustomAffinityMask", "0", maskStr, sizeof(maskStr), iniFilePath.c_str());
        try {
            CustomAffinityMask = std::stoull(maskStr, nullptr, 16);
        }
        catch (...) {
            CustomAffinityMask = 0;
        }

        Logger::Log("ConfigManager: Configuration applied successfully.");
    }
}