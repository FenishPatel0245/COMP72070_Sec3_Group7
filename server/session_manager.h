#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include "logger.h"

namespace fs = std::filesystem;

// ── Structs ────────────────────────────────────────────────────────────────

struct Task {
    std::string name;
    bool        completed = false;
};

struct Session {
    std::string              id;
    std::string              owner;
    std::vector<std::string> members;
    std::vector<Task>        tasks;
    int                      version = 0;   // current image version
    bool                     active  = true;
};

// ── SessionManager ─────────────────────────────────────────────────────────

class SessionManager {
public:
    static SessionManager& instance() {
        static SessionManager inst;
        return inst;
    }

    // ── User auth ──────────────────────────────────────────────────────────
    bool authenticate(const std::string& user, const std::string& pass) {
        // Hardcoded user table – extend as needed
        static std::map<std::string,std::string> users = {
            {"alice", "pass123"}, {"bob", "pass456"},
            {"carol", "pass789"}, {"admin", "admin"}
        };
        auto it = users.find(user);
        bool ok = (it != users.end() && it->second == pass);
        LOG_INFO("Auth attempt: user='" + user + "' result=" + (ok?"OK":"FAIL"));
        return ok;
    }

    // ── Session creation ───────────────────────────────────────────────────
    std::string createSession(const std::string& owner) {
        std::lock_guard<std::mutex> lock(mtx_);
        std::string sid = "session" + std::to_string(++nextId_);
        Session s;
        s.id    = sid;
        s.owner = owner;
        s.members.push_back(owner);
        sessions_[sid] = s;
        LOG_INFO("Session created: " + sid + " by " + owner);
        return sid;
    }

    // ── Join session ───────────────────────────────────────────────────────
    bool joinSession(const std::string& sid, const std::string& user) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = sessions_.find(sid);
        if (it == sessions_.end() || !it->second.active) {
            LOG_WARN("Join failed: session '" + sid + "' not found or inactive");
            return false;
        }
        auto& members = it->second.members;
        if (std::find(members.begin(), members.end(), user) == members.end())
            members.push_back(user);
        LOG_INFO("User '" + user + "' joined session " + sid);
        return true;
    }

    // ── Image upload (simulated) ───────────────────────────────────────────
    std::string uploadImage(const std::string& sid,
                            const std::string& filename,
                            const std::string& content) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = sessions_.find(sid);
        if (it == sessions_.end() || !it->second.active)
            return "";

        it->second.version++;
        int ver = it->second.version;

        // Ensure storage dir exists
        fs::create_directories("storage");

        std::string outName = "storage/" + sid + "_v" + std::to_string(ver) + ".txt";
        std::ofstream ofs(outName);
        ofs << "=== Simulated Image Upload ===\n";
        ofs << "Original file : " << filename << "\n";
        ofs << "Session       : " << sid      << "\n";
        ofs << "Version       : " << ver      << "\n";
        ofs << "Content       :\n" << content  << "\n";
        ofs.close();

        LOG_INFO("Image uploaded: " + outName + " (session=" + sid + " v=" + std::to_string(ver) + ")");
        return outName;
    }

    // ── Timeline ───────────────────────────────────────────────────────────
    std::string getTimeline(const std::string& sid) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = sessions_.find(sid);
        if (it == sessions_.end()) return "Session not found.";

        std::ostringstream oss;
        oss << "Timeline for " << sid << " (versions: " << it->second.version << ")\n";
        for (int v = 1; v <= it->second.version; v++) {
            oss << "  v" << v << " -> storage/" << sid << "_v" << v << ".txt\n";
        }
        if (it->second.version == 0) oss << "  (no uploads yet)\n";
        return oss.str();
    }

    // ── Tasks ──────────────────────────────────────────────────────────────
    bool addTask(const std::string& sid, const std::string& taskName) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = sessions_.find(sid);
        if (it == sessions_.end() || !it->second.active) return false;
        Task t; t.name = taskName; t.completed = false;
        it->second.tasks.push_back(t);
        LOG_INFO("Task added: '" + taskName + "' in session " + sid);
        return true;
    }

    bool completeTask(const std::string& sid, const std::string& taskName) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = sessions_.find(sid);
        if (it == sessions_.end() || !it->second.active) return false;
        for (auto& t : it->second.tasks) {
            if (t.name == taskName) { t.completed = true;
                LOG_INFO("Task completed: '" + taskName + "' in session " + sid);
                return true; }
        }
        LOG_WARN("Task not found: '" + taskName + "' in session " + sid);
        return false;
    }

    std::string listTasks(const std::string& sid) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = sessions_.find(sid);
        if (it == sessions_.end()) return "Session not found.";
        std::ostringstream oss;
        oss << "Tasks for " << sid << ":\n";
        for (auto& t : it->second.tasks)
            oss << "  [" << (t.completed ? "X" : " ") << "] " << t.name << "\n";
        if (it->second.tasks.empty()) oss << "  (none)\n";
        return oss.str();
    }

    // ── End / Archive session ──────────────────────────────────────────────
    bool endSession(const std::string& sid, const std::string& user) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = sessions_.find(sid);
        if (it == sessions_.end()) return false;
        if (it->second.owner != user) {
            LOG_WARN("User '" + user + "' tried to end session '" + sid + "' (not owner)");
            return false;
        }
        it->second.active = false;
        LOG_INFO("Session archived: " + sid + " by " + user);
        return true;
    }

    // List all active sessions
    std::string listSessions() {
        std::lock_guard<std::mutex> lock(mtx_);
        std::ostringstream oss;
        oss << "Active sessions:\n";
        bool any = false;
        for (auto& kv : sessions_) {
            if (kv.second.active) {
                oss << "  " << kv.first << " (owner=" << kv.second.owner
                    << ", members=" << kv.second.members.size() << ")\n";
                any = true;
            }
        }
        if (!any) oss << "  (none)\n";
        return oss.str();
    }

private:
    SessionManager() : nextId_(0) {}
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    std::map<std::string, Session> sessions_;
    std::mutex mtx_;
    int nextId_;
};
