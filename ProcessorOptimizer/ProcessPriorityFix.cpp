#include "ProcessPriorityFix.h"
#include "Logger.h"
#include "ConfigManager.h"
#include <Windows.h>
#include <string>
#include <thread>

namespace ProcessPriorityFix {
    static DWORD g_TargetPriorityClass = NORMAL_PRIORITY_CLASS;
    static bool g_IsWatchdogRunning = false;

    // Background thread to prevent game engines from reverting priority
    void PriorityWatchdog() {
        HANDLE hProcess = GetCurrentProcess();
        while (g_IsWatchdogRunning) {
            DWORD currentPriority = GetPriorityClass(hProcess);

            if (currentPriority != g_TargetPriorityClass && g_TargetPriorityClass != NORMAL_PRIORITY_CLASS) {
                if (SetPriorityClass(hProcess, g_TargetPriorityClass)) {
                    Logger::Log("Watchdog: Process priority was overridden. Restored successfully.");
                }
            }
            // Check every 5 seconds to minimize overhead
            Sleep(5000);
        }
    }

    void Apply(int level) {
        std::string priorityName = "Normal";

        switch (level) {
        case 1: g_TargetPriorityClass = HIGH_PRIORITY_CLASS; priorityName = "High"; break;
        case 2: g_TargetPriorityClass = ABOVE_NORMAL_PRIORITY_CLASS; priorityName = "Above Normal"; break;
        case 3:
            g_TargetPriorityClass = REALTIME_PRIORITY_CLASS;
            priorityName = "Realtime";
            Logger::Log("WARNING: Realtime priority selected. System instability may occur.");
            break;
        default: return;
        }

        // Apply initially
        if (SetPriorityClass(GetCurrentProcess(), g_TargetPriorityClass)) {
            Logger::Log("SUCCESS: Process priority initial setup to " + priorityName + ".");

            // Only start Watchdog if enabled in INI
            if (ConfigManager::EnableWatchdog && !g_IsWatchdogRunning) {
                g_IsWatchdogRunning = true;
                std::thread(PriorityWatchdog).detach();
                Logger::Log("Priority Watchdog thread started.");
            }
            else if (!ConfigManager::EnableWatchdog) {
                Logger::Log("Priority Watchdog is disabled in INI. Skipping thread creation.");
            }
        }
        else {
            Logger::Log("ERROR: Failed to update priority initially. Error: " + std::to_string(GetLastError()));
        }
    }
}