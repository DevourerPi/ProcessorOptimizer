#pragma once
#include <string>

namespace Logger {
    // Initializes the log file and keeps the stream open.
    void Initialize(const std::string& logFilePath);

    // Thread-safe method to write a message to the log file.
    void Log(const std::string& message);

    // Closes the log file gracefully.
    void Shutdown();
}