#include <Windows.h>
#include <string>
#include "UltimateProxyDLL.h"

// Include custom modules
#include "Utils.h"
#include "Logger.h"
#include "ConfigManager.h"
#include "CpuAffinityFix.h"
#include "ProcessPriorityFix.h"
#include "SystemResourceFix.h" // 新增引入

// Main execution thread
DWORD WINAPI MainThread(LPVOID lpParam) {
    HMODULE hModule = (HMODULE)lpParam;

    std::string basePath = Utils::GetCurrentDirectoryPath(hModule);
    std::string logPath = basePath + "\\ProcessorOptimizer.log";
    std::string iniPath = basePath + "\\ProcessorOptimizer.ini";

    // 1. Initialize Logger
    Logger::Initialize(logPath);
    Logger::Log("=== ProcessorOptimizer Core Initialized ===");

    // 2. Load Configuration
    ConfigManager::LoadConfig(iniPath);

    // 3. Engine Initialization Delay
    Logger::Log("Waiting " + std::to_string(ConfigManager::InitDelayMs) + "ms for game engine initialization...");
    Sleep(ConfigManager::InitDelayMs);

    // 4. Apply System Level Fixes (Power & Timer)
    if (ConfigManager::DisableEcoQoS) {
        SystemResourceFix::ApplyPowerThrottlingFix();
    }
    if (ConfigManager::EnableHighPrecisionTimer) {
        SystemResourceFix::ApplyTimerResolutionFix();
    }

    // 5. Apply Process Priority
    if (ConfigManager::PriorityLevel > 0) {
        ProcessPriorityFix::Apply(ConfigManager::PriorityLevel);
    }

    // 6. Apply CPU Affinity / Redistribution
    if (ConfigManager::EnableAffinityFix) {
        CpuAffinityFix::Apply();
    }

    Logger::Log("=== All optimization tasks completed ===");

    // We don't free the library here anymore, because the Proxy DLL 
    // needs to stay alive for the game's lifetime, and our Watchdog thread might be running.
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        upd::create_proxy(hinstDLL);

        // Launch optimization sequence
        CreateThread(nullptr, 0, MainThread, hinstDLL, 0, nullptr);
        break;

    case DLL_PROCESS_DETACH:
        // Cleanup resources before the game closes
        SystemResourceFix::RestoreTimerResolution();
        Logger::Log("=== ProcessorOptimizer Shutting Down ===");
        Logger::Shutdown();

        if (lpvReserved != nullptr) {
            break;
        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}