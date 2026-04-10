#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <cstdio>

namespace Utils {

enum class LogLevel { Debug, Info, Warn, Error };

class Logger {
public:
    static void Init(const std::string& logFile);
    static void Log(LogLevel level, const char* fmt, ...);
    static void Debug(const char* fmt, ...);
    static void Info(const char* fmt, ...);
    static void Warn(const char* fmt, ...);
    static void Error(const char* fmt, ...);
    static void Shutdown();

private:
    static void WriteInternal(LogLevel level, const char* msg);
    static void Write(LogLevel level, const char* msg);
    static const char* LevelToString(LogLevel level);

    static std::ofstream logFile_;
    static std::mutex mutex_;
    static bool initialized_;
};

} // namespace Utils
