#include "logger.h"
#include <cstdarg>
#include <chrono>
#include <ctime>
#include <iostream>

namespace Utils {

std::ofstream Logger::logFile_;
std::mutex Logger::mutex_;
bool Logger::initialized_ = false;

void Logger::Init(const std::string& logFile) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (initialized_) return;
        logFile_.open(logFile, std::ios::out | std::ios::trunc);
        initialized_ = true;
    }
    Info("Logger initialized: %s", logFile.c_str());
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (logFile_.is_open()) {
        logFile_.close();
    }
    initialized_ = false;
}

void Logger::Log(LogLevel level, const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Write(level, buf);
}

void Logger::Debug(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Write(LogLevel::Debug, buf);
}

void Logger::Info(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Write(LogLevel::Info, buf);
}

void Logger::Warn(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Write(LogLevel::Warn, buf);
}

void Logger::Error(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Write(LogLevel::Error, buf);
}

void Logger::WriteInternal(LogLevel level, const char* msg) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm tmBuf;
    localtime_s(&tmBuf, &time);
    
    char timeStr[32];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tmBuf);
    
    char logLine[2048];
    snprintf(logLine, sizeof(logLine), "[%s] [%s] %s\n",
             timeStr, LevelToString(level), msg);
    
    std::cout << logLine;
    
    if (logFile_.is_open()) {
        logFile_ << logLine;
        if (level >= LogLevel::Warn) {
            logFile_.flush();
        }
    }
}

void Logger::Write(LogLevel level, const char* msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    WriteInternal(level, msg);
}

const char* Logger::LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        default: return "UNKNOWN";
    }
}

} // namespace Utils
