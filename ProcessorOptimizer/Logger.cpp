#include "Logger.h"
#include <Windows.h>
#include <fstream>
#include <mutex>

namespace Logger {
    // Static ensures these variables are only visible within this cpp file
    static std::ofstream g_LogFile;
    static std::mutex g_LogMutex;

    void Initialize(const std::string& logFilePath) {
        std::lock_guard<std::mutex> lock(g_LogMutex);
        g_LogFile.open(logFilePath, std::ios::app);
    }

    void Log(const std::string& message) {
        std::lock_guard<std::mutex> lock(g_LogMutex);
        if (g_LogFile.is_open()) {
            SYSTEMTIME st;
            GetLocalTime(&st);

            char timeBuffer[64];
            sprintf_s(timeBuffer, sizeof(timeBuffer), "[%04d-%02d-%02d %02d:%02d:%02d] ",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

            g_LogFile << timeBuffer << message << "\n";
            g_LogFile.flush();
        }
    }

    void Shutdown() {
        std::lock_guard<std::mutex> lock(g_LogMutex);
        if (g_LogFile.is_open()) {
            g_LogFile.close();
        }
    }
}