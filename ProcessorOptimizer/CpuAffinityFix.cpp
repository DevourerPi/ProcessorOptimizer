#include "CpuAffinityFix.h"
#include "Logger.h"
#include "ConfigManager.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <processthreadsapi.h>

namespace CpuAffinityFix {

    // Helper: Converts a standard bitmask into Windows 10 CPU Sets
    bool ApplySoftAffinity(DWORD_PTR mask) {
        HANDLE hProcess = GetCurrentProcess();
        ULONG bufferSize = 0;

        GetSystemCpuSetInformation(nullptr, 0, &bufferSize, hProcess, 0);
        if (bufferSize == 0) return false;

        std::vector<BYTE> buffer(bufferSize);
        if (!GetSystemCpuSetInformation(reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(buffer.data()), bufferSize, &bufferSize, hProcess, 0)) {
            return false;
        }

        std::vector<ULONG> cpuSetIds;
        PSYSTEM_CPU_SET_INFORMATION info = reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(buffer.data());
        size_t count = bufferSize / sizeof(SYSTEM_CPU_SET_INFORMATION);

        for (size_t i = 0; i < count; ++i) {
            if (info[i].Type == CpuSetInformation) {
                if (mask & ((DWORD_PTR)1 << info[i].CpuSet.LogicalProcessorIndex)) {
                    cpuSetIds.push_back(info[i].CpuSet.Id);
                }
            }
        }

        if (SetProcessDefaultCpuSets(hProcess, cpuSetIds.data(), static_cast<ULONG>(cpuSetIds.size()))) {
            return true;
        }
        return false;
    }

    void Apply() {
        Logger::Log("CpuAffinityFix Module Loaded.");
        HANDLE hProcess = GetCurrentProcess();

        // ---------------------------------------------------------
        // SCENARIO 1: Custom Mask is defined in INI
        // ---------------------------------------------------------
        if (ConfigManager::CustomAffinityMask > 0) {
            Logger::Log("Custom affinity mask detected: 0x" + std::to_string(ConfigManager::CustomAffinityMask));

            if (ConfigManager::EnableSoftAffinity) {
                Logger::Log("Attempting to apply via Soft Affinity (CPU Sets)...");
                if (ApplySoftAffinity(ConfigManager::CustomAffinityMask)) {
                    Logger::Log("SUCCESS: Soft Affinity Mask applied.");
                    return;
                }
                Logger::Log("WARNING: Soft Affinity failed/unsupported. Falling back to Hard Affinity.");
            }
            else {
                Logger::Log("Soft Affinity disabled in INI. Proceeding with Hard Affinity.");
            }

            // Fallback: Direct Hard Affinity
            if (SetProcessAffinityMask(hProcess, ConfigManager::CustomAffinityMask)) {
                Logger::Log("SUCCESS: Hard Affinity Mask applied.");
            }
            else {
                Logger::Log("ERROR: Failed to apply Hard Affinity. Error: " + std::to_string(GetLastError()));
            }
            return;
        }

        // ---------------------------------------------------------
        // SCENARIO 2: No Custom Mask (Default Core Redistribution)
        // ---------------------------------------------------------
        DWORD_PTR processMask, systemMask;
        if (!GetProcessAffinityMask(hProcess, &processMask, &systemMask)) {
            Logger::Log("ERROR: Failed to retrieve process affinity mask.");
            return;
        }

        // Try modern Soft Affinity first (offloading Core 0)
        if (ConfigManager::EnableSoftAffinity) {
            Logger::Log("Applying Core 0 offloading via Soft Affinity...");
            DWORD_PTR optimizedMask = processMask & ~1ULL; // Remove Core 0

            if (optimizedMask != 0) {
                if (ApplySoftAffinity(optimizedMask)) {
                    Logger::Log("SUCCESS: Core 0 offloaded via Soft Affinity.");
                    return;
                }
                Logger::Log("WARNING: Soft Affinity failed. Falling back to classic cycle trick.");
            }
        }
        else {
            Logger::Log("Soft Affinity disabled. Proceeding with classic Hard Affinity cycle trick.");
        }

        // ---------------------------------------------------------
        // SCENARIO 3: The Classic "Cycle Trick" (Your original method)
        // ---------------------------------------------------------
        Logger::Log("Starting multi-core redistribution trick (Hard Affinity Cycle)...");
        int maxBits = sizeof(DWORD_PTR) * 8;
        int coresProcessed = 0;

        for (int i = 0; i < maxBits; ++i) {
            DWORD_PTR currentCoreMask = ((DWORD_PTR)1 << i);

            if (processMask & currentCoreMask) {
                DWORD_PTR tempMask = processMask & ~currentCoreMask;

                if (tempMask == 0) continue; // Safety check

                if (SetProcessAffinityMask(hProcess, tempMask)) {
                    Logger::Log("SUCCESS: Temporarily removed Core " + std::to_string(i) + ".");
                    Sleep(50);

                    SetProcessAffinityMask(hProcess, processMask);
                    Sleep(50);
                }
                coresProcessed++;
            }
        }
        Logger::Log("CpuAffinityFix classic routine completed. Processed " + std::to_string(coresProcessed) + " active cores.");
    }
}