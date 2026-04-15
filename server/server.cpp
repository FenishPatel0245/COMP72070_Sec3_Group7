// ============================================================
//  BoardNova Server  –  Multi-client TCP whiteboard server
//  Compile:  g++ server.cpp -o server -lws2_32 -lstdc++fs -std=c++17
//  Run:      server.exe
// ============================================================
#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET sock_t;
  #define CLOSE_SOCK(s) closesocket(s)
  #define SOCK_INVALID  INVALID_SOCKET
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  typedef int sock_t;
  #define CLOSE_SOCK(s) close(s)
  #define SOCK_INVALID  (-1)
#endif

#include <iostream>
#include <thread>
#include <string>
#include <sstream>
#include <cstring>

#include "../common/packet.h"
#include "logger.h"
#include "state_machine.h"
#include "session_manager.h"

// ── Config ─────────────────────────────────────────────────
const int    PORT       = 8080;
const int    BACKLOG    = 10;
const size_t BUF_SIZE   = 4096;

// ── Banner ─────────────────────────────────────────────────
void printBanner() {
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════╗\n";
    std::cout << "  ║    BoardNova Whiteboard Server v1.0      ║\n";
    std::cout << "  ║    Listening on port " << PORT << "               ║\n";
    std::cout << "  ╚══════════════════════════════════════════╝\n\n";
}

// ── Send a Packet ──────────────────────────────────────────
void sendPacket(sock_t sock, const Packet& p) {
    std::string data = serializePacket(p);
    send(sock, data.c_str(), (int)data.size(), 0);
}

// ── Receive one line ───────────────────────────────────────
std::string recvLine(sock_t sock) {
    std::string result;
    char ch = 0;
    while (true) {
        int n = recv(sock, &ch, 1, 0);
        if (n <= 0) break;
        result += ch;
        if (ch == '\n') break;
    }
    return result;
}

// ── Handle one client connection ───────────────────────────
void handleClient(sock_t clientSock, int clientId) {
    std::string cid = "Client#" + std::to_string(clientId);
    LOG_INFO(cid + " connected.");

    ClientState state     = STATE_IDLE;
    std::string username;
    std::string sessionId;

    while (true) {
        std::string raw = recvLine(clientSock);
        if (raw.empty()) {
            LOG_INFO(cid + " disconnected.");
            break;
        }

        Packet pkt;
        try {
            pkt = deserializePacket(raw);
        } catch (...) {
            sendPacket(clientSock, makeError("Malformed packet."));
            continue;
        }

        // ── Dispatch ────────────────────────────────────────
        switch (pkt.type) {

        // ── LOGIN ──────────────────────────────────────────
        case PKT_LOGIN: {
            if (state != STATE_IDLE) {
                sendPacket(clientSock, makeError("Already authenticated."));
                break;
            }
            // payload: "username password"
            std::istringstream ss(pkt.payload);
            std::string user, pass;
            ss >> user >> pass;

            auto& sm = SessionManager::instance();
            if (sm.authenticate(user, pass)) {
                username = user;
                transition(state, STATE_AUTHENTICATED, cid);
                sendPacket(clientSock, makeResponse("Welcome, " + username + "!"));
            } else {
                sendPacket(clientSock, makeError("Invalid credentials."));
            }
            break;
        }

        // ── CREATE SESSION ─────────────────────────────────
        case PKT_CREATE_SESSION: {
            if (state != STATE_AUTHENTICATED) {
                sendPacket(clientSock, makeError("Not authenticated or already in a session."));
                break;
            }
            auto& sm = SessionManager::instance();
            sessionId = sm.createSession(username);
            transition(state, STATE_SESSION_ACTIVE, cid);
            sendPacket(clientSock, makeResponse("Session created: " + sessionId));
            break;
        }

        // ── JOIN SESSION ───────────────────────────────────
        case PKT_JOIN_SESSION: {
            if (state != STATE_AUTHENTICATED) {
                sendPacket(clientSock, makeError("Not authenticated."));
                break;
            }
            std::string sid = pkt.payload;
            auto& sm = SessionManager::instance();
            if (sm.joinSession(sid, username)) {
                sessionId = sid;
                transition(state, STATE_SESSION_ACTIVE, cid);
                sendPacket(clientSock, makeResponse("Joined session: " + sessionId));
            } else {
                sendPacket(clientSock, makeError("Session not found or inactive: " + sid));
            }
            break;
        }

        // ── LIST SESSIONS ─────────────────────────────────-
        case PKT_LIST_SESSIONS: {
            auto& sm = SessionManager::instance();
            sendPacket(clientSock, makeResponse(sm.listSessions()));
            break;
        }

        // ── UPLOAD IMAGE ───────────────────────────────────
        case PKT_UPLOAD_IMAGE: {
            if (state != STATE_SESSION_ACTIVE) {
                sendPacket(clientSock, makeError("Not in an active session."));
                break;
            }
            transition(state, STATE_RECEIVING_IMAGE, cid);
            // metadata = filename, payload = simulated content
            std::string filename = pkt.metadata;
            std::string content  = pkt.payload;
            auto& sm = SessionManager::instance();
            std::string outPath = sm.uploadImage(sessionId, filename, content);
            transition(state, STATE_VERSIONING, cid);
            // Back to session active
            transition(state, STATE_SESSION_ACTIVE, cid);
            if (!outPath.empty()) {
                sendPacket(clientSock, makeResponse("Image saved: " + outPath));
            } else {
                sendPacket(clientSock, makeError("Upload failed."));
            }
            break;
        }

        // ── VIEW TIMELINE ──────────────────────────────────
        case PKT_VIEW_TIMELINE: {
            if (state != STATE_SESSION_ACTIVE) {
                sendPacket(clientSock, makeError("Not in an active session."));
                break;
            }
            auto& sm = SessionManager::instance();
            sendPacket(clientSock, makeResponse(sm.getTimeline(sessionId)));
            break;
        }

        // ── ADD TASK ───────────────────────────────────────
        case PKT_ADD_TASK: {
            if (state != STATE_SESSION_ACTIVE) {
                sendPacket(clientSock, makeError("Not in an active session."));
                break;
            }
            transition(state, STATE_TASK_UPDATE, cid);
            auto& sm = SessionManager::instance();
            if (sm.addTask(sessionId, pkt.payload)) {
                transition(state, STATE_SESSION_ACTIVE, cid);
                sendPacket(clientSock, makeResponse("Task added: " + pkt.payload));
            } else {
                transition(state, STATE_SESSION_ACTIVE, cid);
                sendPacket(clientSock, makeError("Failed to add task."));
            }
            break;
        }

        // ── COMPLETE TASK ──────────────────────────────────
        case PKT_COMPLETE_TASK: {
            if (state != STATE_SESSION_ACTIVE) {
                sendPacket(clientSock, makeError("Not in an active session."));
                break;
            }
            transition(state, STATE_TASK_UPDATE, cid);
            auto& sm = SessionManager::instance();
            if (sm.completeTask(sessionId, pkt.payload)) {
                transition(state, STATE_SESSION_ACTIVE, cid);
                sendPacket(clientSock, makeResponse("Task completed: " + pkt.payload));
            } else {
                transition(state, STATE_SESSION_ACTIVE, cid);
                sendPacket(clientSock, makeError("Task not found: " + pkt.payload));
            }
            break;
        }

        // ── END SESSION ────────────────────────────────────
        case PKT_END_SESSION: {
            if (state != STATE_SESSION_ACTIVE) {
                sendPacket(clientSock, makeError("Not in an active session."));
                break;
            }
            transition(state, STATE_ARCHIVING, cid);
            auto& sm = SessionManager::instance();
            if (sm.endSession(sessionId, username)) {
                transition(state, STATE_AUTHENTICATED, cid);
                sendPacket(clientSock, makeResponse("Session " + sessionId + " ended and archived."));
                sessionId.clear();
            } else {
                transition(state, STATE_SESSION_ACTIVE, cid);
                sendPacket(clientSock, makeError("Only session owner can end the session."));
            }
            break;
        }

        default:
            sendPacket(clientSock, makeError("Unknown packet type."));
            break;
        }
    }

    CLOSE_SOCK(clientSock);
    LOG_INFO(cid + " connection closed.");
}

// ── Main ───────────────────────────────────────────────────
int main() {
    printBanner();
    LOG_INFO("BoardNova Server starting...");

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        LOG_ERROR("WSAStartup failed.");
        return 1;
    }
#endif

    sock_t serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == SOCK_INVALID) {
        LOG_ERROR("Failed to create socket.");
        return 1;
    }

    // Allow port reuse
    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    if (bind(serverSock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("Bind failed on port " + std::to_string(PORT));
        CLOSE_SOCK(serverSock);
        return 1;
    }

    if (listen(serverSock, BACKLOG) < 0) {
        LOG_ERROR("Listen failed.");
        CLOSE_SOCK(serverSock);
        return 1;
    }

    LOG_INFO("Server listening on port " + std::to_string(PORT) + ". Waiting for clients...");
    std::cout << "  [Ready] Press Ctrl+C to stop.\n\n";

    int clientId = 0;
    while (true) {
        sockaddr_in clientAddr{};
        int addrLen = sizeof(clientAddr);
        sock_t clientSock = accept(serverSock, (sockaddr*)&clientAddr, &addrLen);
        if (clientSock == SOCK_INVALID) continue;

        char ipBuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuf, sizeof(ipBuf));
        LOG_INFO("New connection from " + std::string(ipBuf) +
                 " assigned ID=" + std::to_string(++clientId));

        std::thread(handleClient, clientSock, clientId).detach();
    }

    CLOSE_SOCK(serverSock);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
