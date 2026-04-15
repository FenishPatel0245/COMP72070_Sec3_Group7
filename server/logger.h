#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <ctime>
#include <iostream>
#include <sstream>

class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    void log(const std::string& level, const std::string& message) {
        std::lock_guard<std::mutex> lock(mtx_);
        std::string entry = "[" + timestamp() + "][" + level + "] " + message;
        std::cout << entry << std::endl;
        if (file_.is_open()) {
            file_ << entry << std::endl;
            file_.flush();
        }
    }

    void info(const std::string& msg)  { log("INFO ", msg); }
    void warn(const std::string& msg)  { log("WARN ", msg); }
    void error(const std::string& msg) { log("ERROR", msg); }

private:
    Logger() {
        file_.open("logs.txt", std::ios::app);
        if (!file_.is_open()) {
            std::cerr << "[Logger] Warning: could not open logs.txt for writing.\n";
        }
    }
    ~Logger() { if (file_.is_open()) file_.close(); }
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string timestamp() {
        std::time_t now = std::time(nullptr);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        return std::string(buf);
    }

    std::ofstream file_;
    std::mutex    mtx_;
};

// Convenience macros
#define LOG_INFO(msg)  Logger::instance().info(msg)
#define LOG_WARN(msg)  Logger::instance().warn(msg)
#define LOG_ERROR(msg) Logger::instance().error(msg)
