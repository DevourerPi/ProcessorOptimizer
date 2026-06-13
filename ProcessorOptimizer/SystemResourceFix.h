#pragma once

namespace SystemResourceFix {
    // Disables Windows EcoQoS (Power Throttling) for this process to ensure maximum performance.
    void ApplyPowerThrottlingFix();

    // Forces the Windows global timer resolution to 1ms to reduce thread wake-up latency.
    void ApplyTimerResolutionFix();

    // Restores the timer resolution to default before the DLL detaches.
    void RestoreTimerResolution();
}