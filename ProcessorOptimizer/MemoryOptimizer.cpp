#include "MemoryOptimizer.h"
#include "Logger.h"
#include "ConfigManager.h"
#include <Windows.h>
#include <psapi.h>
#include <string>
#include <thread>
#include <vector>

// Link the Process Status API library required for GetProcessMemoryInfo
#pragma comment(lib, "psapi.lib")

namespace MemoryOptimizer {

    // Global flag to control the background monitoring thread lifecycle
    static bool g_IsWatchdogRunning = false;

    // ========================================================================
    // Phase 1: Low Fragmentation Heap (LFH)
    // ========================================================================
    // Enables LFH for all active heaps in the process.
    // This prevents long-term memory fragmentation in older game engines 
    // by optimizing how small memory blocks are allocated.
    void ApplyLowFragmentationHeap() {
        DWORD heapCount = GetProcessHeaps(0, nullptr);
        if (heapCount == 0) return;

        std::vector<HANDLE> heaps(heapCount);
        heapCount = GetProcessHeaps(heapCount, heaps.data());

        ULONG heapFragValue = 2; // 2 activates Low Fragmentation Heap (LFH)
        int successCount = 0;

        for (DWORD i = 0; i < heapCount; ++i) {
            // Attempt to apply the LFH policy to each individual heap
            if (HeapSetInformation(heaps[i], HeapCompatibilityInformation, &heapFragValue, sizeof(heapFragValue))) {
                successCount++;
            }
        }
        Logger::Log("MemoryOptimizer: LFH activated on " + std::to_string(successCount) + "/" + std::to_string(heapCount) + " heaps.");
    }

    // ========================================================================
    // Phase 2: Execution of Memory Trimming
    // ========================================================================
    // Performs a gentle working set trim using advanced Win32 flags if enabled.
    void PerformGentleTrim() {
        HANDLE hProcess = GetCurrentProcess();

        if (ConfigManager::EnableSmartTrim) {
            // QUOTA_LIMITS_HARDWS_MIN_DISABLE allows the OS to smoothly reclaim long-unused pages
            // without forcefully paging out active game assets. This ensures zero stutter.
            DWORD flags = QUOTA_LIMITS_HARDWS_MIN_DISABLE | QUOTA_LIMITS_HARDWS_MAX_DISABLE;

            if (SetProcessWorkingSetSizeEx(hProcess, (SIZE_T)-1, (SIZE_T)-1, flags)) {
                Logger::Log("MemoryOptimizer: Smart Working Set Trim executed smoothly.");
            }
            else {
                Logger::Log("MemoryOptimizer ERROR: Smart Trim failed. Error: " + std::to_string(GetLastError()) + ". Falling back to classic method.");
                // Fallback if the advanced API fails for any reason
                SetProcessWorkingSetSize(hProcess, (SIZE_T)-1, (SIZE_T)-1);
            }
        }
        else {
            // Legacy/Standard Trim (Forceful, aggressive, might cause a brief frame stutter)
            if (SetProcessWorkingSetSize(hProcess, (SIZE_T)-1, (SIZE_T)-1)) {
                Logger::Log("MemoryOptimizer: Standard Working Set Trim executed.");
            }
        }
    }

    // ========================================================================
    // Phase 3: The Background Daemon (Watchdog)
    // ========================================================================
    // Background thread to intelligently monitor memory bloat and page fault rates.
    void MemoryMonitorWatchdog() {
        HANDLE hProcess = GetCurrentProcess();
        PROCESS_MEMORY_COUNTERS_EX pmc;

        DWORD quietTimerSeconds = 0;
        ULONG lastPageFaultCount = 0;

        // Convert the user-defined threshold from Megabytes (MB) to Bytes
        SIZE_T thresholdBytes = static_cast<SIZE_T>(ConfigManager::BloatThresholdMB) * 1024 * 1024;

        Logger::Log("Memory Watchdog daemon thread started.");

        while (g_IsWatchdogRunning) {
            // Sleep for 1 second intervals. This keeps the CPU overhead at an absolute 0%.
            Sleep(1000);

            if (!GetProcessMemoryInfo(hProcess, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
                continue; // Skip this cycle if we fail to read process memory info
            }

            SIZE_T currentWorkingSet = pmc.WorkingSetSize;
            ULONG currentPageFaults = pmc.PageFaultCount;

            // Calculate how many page faults occurred in the exact last 1 second
            ULONG pageFaultsThisSecond = (lastPageFaultCount == 0) ? 0 : (currentPageFaults - lastPageFaultCount);
            lastPageFaultCount = currentPageFaults;

            // Check Condition 1: Has the game memory exceeded the bloat threshold?
            if (currentWorkingSet > thresholdBytes) {

                // Check Condition 2: Is the game currently in a 'Quiet Period'? 
                // A very low page fault rate (< 50) indicates the engine isn't aggressively 
                // streaming assets (e.g., the player is in a menu, paused, or standing still).
                if (pageFaultsThisSecond < 50) {
                    quietTimerSeconds++;
                }
                else {
                    // Reset timer if the game starts actively loading data to prevent stuttering
                    quietTimerSeconds = 0;
                }

                // If both conditions are met for the required duration, trigger safe optimization
                if (quietTimerSeconds >= ConfigManager::QuietPeriodSeconds) {
                    Logger::Log("Memory Alert: Working Set (" + std::to_string(currentWorkingSet / 1024 / 1024) +
                        " MB) exceeded threshold. Engine is quiet. Triggering optimization...");

                    PerformGentleTrim();

                    // Reset the timer and wait for the next memory bloat cycle
                    quietTimerSeconds = 0;
                }
            }
            else {
                // Reset the quiet timer if memory drops below the threshold naturally
                quietTimerSeconds = 0;
            }
        }
    }

    // ========================================================================
    // Module Initialization
    // ========================================================================
    // Activates the memory optimization pipeline based on user configuration.
    void Initialize() {
        // Master switch check
        if (!ConfigManager::EnableMemoryOptimization) {
            Logger::Log("MemoryOptimizer: Disabled in configuration. Bypassing module.");
            return;
        }

        // Apply Heap Optimization immediately upon injection
        if (ConfigManager::EnableLFH) {
            ApplyLowFragmentationHeap();
        }

        // Spawn the continuous monitoring daemon in a detached background thread
        if (!g_IsWatchdogRunning) {
            g_IsWatchdogRunning = true;
            std::thread(MemoryMonitorWatchdog).detach();
        }
    }
}