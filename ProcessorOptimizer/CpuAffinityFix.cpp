#include "CpuAffinityFix.h"
#include "Logger.h"
#include "ConfigManager.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <processthreadsapi.h>

namespace CpuAffinityFix {

    // 底层软相关性实现
    bool InternalApplySoftAffinity(DWORD_PTR mask) {
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
        return SetProcessDefaultCpuSets(hProcess, cpuSetIds.data(), static_cast<ULONG>(cpuSetIds.size())) != 0;
    }

    // 统一分配器：根据配置决定使用 Soft 还是 Hard
    bool ApplyMask(DWORD_PTR mask, bool useSoftAPI, const std::string& actionDesc) {
        if (mask == 0) return false; // 安全检查

        if (useSoftAPI) {
            if (InternalApplySoftAffinity(mask)) {
                Logger::Log(actionDesc + " (Soft API - Success)");
                return true;
            }
            else {
                Logger::Log(actionDesc + " (Soft API - Failed, falling back to Hard API)");
            }
        }

        // Hard API (Fallback or explicitly requested)
        if (SetProcessAffinityMask(GetCurrentProcess(), mask)) {
            Logger::Log(actionDesc + " (Hard API - Success)");
            return true;
        }

        Logger::Log(actionDesc + " (Failed completely. Error: " + std::to_string(GetLastError()) + ")");
        return false;
    }

    void Apply() {
        Logger::Log("CpuAffinityFix Module Loaded. User Architecture Engaged.");
        HANDLE hProcess = GetCurrentProcess();
        DWORD_PTR processMask, systemMask;

        if (!GetProcessAffinityMask(hProcess, &processMask, &systemMask)) {
            Logger::Log("ERROR: Failed to retrieve base process affinity mask.");
            return;
        }

        bool useSoft = ConfigManager::EnableSoftAffinity;

        // ==========================================
        // 阶段 1：解绑与恢复操作 (The Shake Up Phase)
        // ==========================================
        if (ConfigManager::EnableAffinityFix == 1) {
            Logger::Log("Executing Phase 1: Unbind/Restore Core 0 only.");
            DWORD_PTR tempMask = processMask & ~1ULL; // 移除 Core 0

            if (ApplyMask(tempMask, useSoft, "Temporarily Unbind Core 0")) {
                Sleep(50); // 注意：如果使用 Soft API，50ms可能不足以让系统响应
                ApplyMask(processMask, useSoft, "Restore Core 0");
            }

        }
        else if (ConfigManager::EnableAffinityFix == 2) {
            Logger::Log("Executing Phase 1: Unbind/Restore ALL active cores sequentially.");
            int maxBits = sizeof(DWORD_PTR) * 8;

            for (int i = 0; i < maxBits; ++i) {
                DWORD_PTR currentCoreMask = ((DWORD_PTR)1 << i);
                if (processMask & currentCoreMask) {
                    DWORD_PTR tempMask = processMask & ~currentCoreMask;
                    if (tempMask == 0) continue;

                    if (ApplyMask(tempMask, useSoft, "Temporarily Unbind Core " + std::to_string(i))) {
                        Sleep(50);
                        ApplyMask(processMask, useSoft, "Restore Core " + std::to_string(i));
                        Sleep(50);
                    }
                }
            }
        }
        else {
            Logger::Log("Phase 1 Bypassed: EnableAffinityFix is 0.");
        }

        // ==========================================
        // 阶段 2：应用最终用户掩码 (The Final Override)
        // ==========================================
        if (ConfigManager::CustomAffinityMask > 0) {
            Logger::Log("Executing Phase 2: Applying Custom Affinity Mask: 0x" + std::to_string(ConfigManager::CustomAffinityMask));
            ApplyMask(ConfigManager::CustomAffinityMask, useSoft, "Set Final Custom Mask");
        }
        else {
            Logger::Log("Phase 2 Bypassed: No Custom Mask defined.");
        }

        Logger::Log("CpuAffinityFix routine completed.");
    }
}