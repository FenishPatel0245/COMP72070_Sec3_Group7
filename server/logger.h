#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <ctime>
#include <iostream>
#include <sstream>

#ifdef _WIN32
    #include <windows.h>
#endif

class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    void log(const std::string& level, const std::string& message) {
#ifdef _WIN32
        EnterCriticalSection(&cs_);
#endif
        std::string entry = "[" + timestamp() + "][" + level + "] " + message;
        std::cout << entry << std::endl;
        if (file_.is_open()) {
            file_ << entry << std::endl;
            file_.flush();
        }
#ifdef _WIN32
        LeaveCriticalSection(&cs_);
#endif
    }

    void info(const std::string& msg)  { log("INFO ", msg); }
    void warn(const std::string& msg)  { log("WARN ", msg); }
    void error(const std::string& msg) { log("ERROR", msg); }

private:
    Logger() {
        file_.open("logs.txt", std::ios::app);
#ifdef _WIN32
        InitializeCriticalSection(&cs_);
#endif
        if (!file_.is_open()) {
            std::cerr << "[Logger] Warning: could not open logs.txt for writing.\n";
        }
    }
    ~Logger() { 
        if (file_.is_open()) file_.close(); 
#ifdef _WIN32
        DeleteCriticalSection(&cs_);
#endif
    }
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string timestamp() {
        std::time_t now = std::time(nullptr);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        return std::string(buf);
    }

    std::ofstream file_;
#ifdef _WIN32
    CRITICAL_SECTION cs_;
#else
    std::mutex    mtx_; // Fallback for non-windows
#endif
};

// Convenience macros
#define LOG_INFO(msg)  Logger::instance().info(msg)
#define LOG_WARN(msg)  Logger::instance().warn(msg)
#define LOG_ERROR(msg) Logger::instance().error(msg)
