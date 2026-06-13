#include "SystemResourceFix.h"
#include "Logger.h"
#include <Windows.h>
#include <string>

// Required library for timeBeginPeriod / timeEndPeriod
#pragma comment(lib, "winmm.lib")
#include <timeapi.h>

namespace SystemResourceFix {

    static bool g_TimerModified = false;

    void ApplyPowerThrottlingFix() {
        // PROCESS_POWER_THROTTLING_STATE is available starting from Windows 10, version 1709
        PROCESS_POWER_THROTTLING_STATE powerState = { 0 };
        powerState.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;

        // We want to control the execution speed limit
        powerState.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
        // Setting StateMask to 0 disables the throttling (turns off EcoQoS)
        powerState.StateMask = 0;

        if (SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &powerState, sizeof(powerState))) {
            Logger::Log("SUCCESS: Windows Power Throttling (EcoQoS) disabled. Forcing maximum performance state.");
        }
        else {
            // Note: This might naturally fail on older Windows versions (e.g., Win7)
            Logger::Log("WARNING: Failed to disable Power Throttling. OS might not support it. Error: " + std::to_string(GetLastError()));
        }
    }

    void ApplyTimerResolutionFix() {
        // Request a 1 millisecond minimum resolution
        if (timeBeginPeriod(1) == TIMERR_NOERROR) {
            g_TimerModified = true;
            Logger::Log("SUCCESS: Global timer resolution set to 1ms (High Precision).");
        }
        else {
            Logger::Log("ERROR: Failed to set high precision timer.");
        }
    }

    void RestoreTimerResolution() {
        if (g_TimerModified) {
            timeEndPeriod(1);
            Logger::Log("SystemResourceFix: Restored global timer resolution.");
            g_TimerModified = false;
        }
    }
}