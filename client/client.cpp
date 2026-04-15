// ============================================================
//  BoardNova Client  –  Console UI for the whiteboard system
//  Compile:  g++ client.cpp -o client -lws2_32 -std=c++17
//  Run:      client.exe
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
#include <string>
#include <sstream>
#include <fstream>
#include <limits>
#include "../common/packet.h"

// ── Config ─────────────────────────────────────────────────
const char* SERVER_IP   = "127.0.0.1";
const int   SERVER_PORT = 8080;

// ── Colours (Windows ANSI) ─────────────────────────────────
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define CYAN    "\033[96m"
#define GREEN   "\033[92m"
#define YELLOW  "\033[93m"
#define RED     "\033[91m"
#define BLUE    "\033[94m"
#define MAGENTA "\033[95m"

// ── Helpers ────────────────────────────────────────────────
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

void enableAnsi() {
#ifdef _WIN32
    // Enable VT processing for colours on Windows
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(h, &mode);
    SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pause() {
    std::cout << YELLOW << "\n  Press ENTER to continue..." << RESET;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void printDivider(char c = '-', int len = 50) {
    std::cout << CYAN;
    for (int i = 0; i < len; i++) std::cout << c;
    std::cout << RESET << "\n";
}

void printSuccess(const std::string& msg) {
    std::cout << GREEN << "  [OK] " << msg << RESET << "\n";
}

void printError(const std::string& msg) {
    std::cout << RED << "  [ERR] " << msg << RESET << "\n";
}

void printInfo(const std::string& msg) {
    std::cout << BLUE << "  " << msg << RESET << "\n";
}

// ── Network ────────────────────────────────────────────────

sock_t gSock = SOCK_INVALID;

bool netInit() {
#ifdef _WIN32
    WSADATA wsa;
    return (WSAStartup(MAKEWORD(2,2), &wsa) == 0);
#else
    return true;
#endif
}

void netCleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

bool connectToServer() {
    gSock = socket(AF_INET, SOCK_STREAM, 0);
    if (gSock == SOCK_INVALID) return false;

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    return (connect(gSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == 0);
}

void sendPkt(const Packet& p) {
    std::string data = serializePacket(p);
    send(gSock, data.c_str(), (int)data.size(), 0);
}

Packet recvPkt() {
    std::string result;
    char ch = 0;
    while (true) {
        int n = recv(gSock, &ch, 1, 0);
        if (n <= 0) break;
        result += ch;
        if (ch == '\n') break;
    }
    if (result.empty()) {
        Packet p; p.type = PKT_ERROR; p.payload = "Connection lost."; return p;
    }
    return deserializePacket(result);
}

// Send a request and print the server's response; return true on PKT_RESPONSE
bool doRequest(const Packet& req) {
    sendPkt(req);
    Packet resp = recvPkt();
    if (resp.type == PKT_RESPONSE) {
        printSuccess(resp.payload);
        return true;
    } else {
        printError(resp.payload);
        return false;
    }
}

// ── Menu Banner ────────────────────────────────────────────
void printBanner(const std::string& user, const std::string& session) {
    clearScreen();
    printDivider('=');
    std::cout << BOLD << MAGENTA
              << "       BoardNova  –  Whiteboard System\n"
              << RESET;
    printDivider('=');
    if (!user.empty())
        std::cout << GREEN << "  Logged in as : " << BOLD << user << RESET << "\n";
    else
        std::cout << YELLOW << "  Not logged in\n" << RESET;
    if (!session.empty())
        std::cout << CYAN << "  Session ID   : " << BOLD << session << RESET << "\n";
    printDivider();
    std::cout << BOLD
              << "  1.  Login\n"
              << "  2.  Create Session\n"
              << "  3.  Join Session\n"
              << "  4.  List Sessions\n"
              << "  5.  Upload Whiteboard Image\n"
              << "  6.  View Timeline\n"
              << "  7.  Add Task\n"
              << "  8.  Complete Task\n"
              << "  9.  End Session\n"
              << "  0.  Exit\n"
              << RESET;
    printDivider();
    std::cout << CYAN << "  Enter choice: " << RESET;
}

// ── Input helper ───────────────────────────────────────────
std::string prompt(const std::string& label) {
    std::cout << YELLOW << "  " << label << ": " << RESET;
    std::string val;
    std::getline(std::cin, val);
    return val;
}

// ── Action handlers ────────────────────────────────────────

std::string gUser, gSession;

void doLogin() {
    std::cout << "\n";
    printDivider();
    std::cout << BOLD << "  [ LOGIN ]\n" << RESET;
    printDivider();
    std::string user = prompt("Username");
    std::string pass = prompt("Password");

    Packet p;
    p.type     = PKT_LOGIN;
    p.metadata = "login";
    p.payload  = user + " " + pass;
    p.length   = (int)p.payload.size();

    sendPkt(p);
    Packet resp = recvPkt();
    if (resp.type == PKT_RESPONSE) {
        gUser = user;
        printSuccess(resp.payload);
    } else {
        printError(resp.payload);
    }
}

void doCreateSession() {
    if (gUser.empty()) { printError("Please login first."); pause(); return; }
    std::cout << "\n";
    printDivider();
    std::cout << BOLD << "  [ CREATE SESSION ]\n" << RESET;
    printDivider();

    Packet p;
    p.type     = PKT_CREATE_SESSION;
    p.metadata = "create";
    p.payload  = "";
    p.length   = 0;

    sendPkt(p);
    Packet resp = recvPkt();
    if (resp.type == PKT_RESPONSE) {
        // Extract session id from response like "Session created: session1"
        std::string msg = resp.payload;
        auto pos = msg.rfind(' ');
        if (pos != std::string::npos) gSession = msg.substr(pos + 1);
        printSuccess(msg);
    } else {
        printError(resp.payload);
    }
}

void doJoinSession() {
    if (gUser.empty()) { printError("Please login first."); pause(); return; }
    std::cout << "\n";
    printDivider();
    std::cout << BOLD << "  [ JOIN SESSION ]\n" << RESET;
    printDivider();
    std::string sid = prompt("Session ID (e.g. session1)");

    Packet p;
    p.type     = PKT_JOIN_SESSION;
    p.metadata = "join";
    p.payload  = sid;
    p.length   = (int)sid.size();

    sendPkt(p);
    Packet resp = recvPkt();
    if (resp.type == PKT_RESPONSE) {
        gSession = sid;
        printSuccess(resp.payload);
    } else {
        printError(resp.payload);
    }
}

void doListSessions() {
    Packet p;
    p.type     = PKT_LIST_SESSIONS;
    p.metadata = "list";
    p.payload  = "";
    p.length   = 0;

    sendPkt(p);
    Packet resp = recvPkt();
    std::cout << "\n";
    printDivider();
    std::cout << BOLD << "  [ ACTIVE SESSIONS ]\n" << RESET;
    printDivider();
    if (resp.type == PKT_RESPONSE)
        printInfo(resp.payload);
    else
        printError(resp.payload);
}

void doUploadImage() {
    if (gSession.empty()) { printError("Join or create a session first."); pause(); return; }
    std::cout << "\n";
    printDivider();
    std::cout << BOLD << "  [ UPLOAD IMAGE ]\n" << RESET;
    printDivider();
    std::string fname = prompt("Filename to upload (e.g. board.txt)");

    // Try to read file; if not found, use placeholder content
    std::string content;
    std::ifstream f(fname);
    if (f.is_open()) {
        std::ostringstream oss;
        oss << f.rdbuf();
        content = oss.str();
        f.close();
    } else {
        content = "[Simulated image data for file: " + fname + "]";
        printInfo("(File not found locally – sending simulated content)");
    }

    Packet p;
    p.type     = PKT_UPLOAD_IMAGE;
    p.metadata = fname;
    p.payload  = content;
    p.length   = (int)content.size();

    if (doRequest(p)) {}
}

void doViewTimeline() {
    if (gSession.empty()) { printError("Join or create a session first."); pause(); return; }
    std::cout << "\n";
    printDivider();
    std::cout << BOLD << "  [ TIMELINE ]\n" << RESET;
    printDivider();

    Packet p;
    p.type     = PKT_VIEW_TIMELINE;
    p.metadata = "timeline";
    p.payload  = "";
    p.length   = 0;

    sendPkt(p);
    Packet resp = recvPkt();
    if (resp.type == PKT_RESPONSE)
        printInfo(resp.payload);
    else
        printError(resp.payload);
}

void doAddTask() {
    if (gSession.empty()) { printError("Join or create a session first."); pause(); return; }
    std::cout << "\n";
    printDivider();
    std::cout << BOLD << "  [ ADD TASK ]\n" << RESET;
    printDivider();
    std::string taskName = prompt("Task name");

    Packet p;
    p.type     = PKT_ADD_TASK;
    p.metadata = "task";
    p.payload  = taskName;
    p.length   = (int)taskName.size();

    doRequest(p);
}

void doCompleteTask() {
    if (gSession.empty()) { printError("Join or create a session first."); pause(); return; }
    std::cout << "\n";
    printDivider();
    std::cout << BOLD << "  [ COMPLETE TASK ]\n" << RESET;
    printDivider();
    std::string taskName = prompt("Task name to complete");

    Packet p;
    p.type     = PKT_COMPLETE_TASK;
    p.metadata = "task";
    p.payload  = taskName;
    p.length   = (int)taskName.size();

    doRequest(p);
}

void doEndSession() {
    if (gSession.empty()) { printError("Not in a session."); pause(); return; }
    std::cout << "\n";
    printDivider();
    std::cout << BOLD << "  [ END SESSION ]\n" << RESET;
    printDivider();
    std::string confirm = prompt("End session " + gSession + "? (yes/no)");
    if (confirm != "yes") { printInfo("Cancelled."); pause(); return; }

    Packet p;
    p.type     = PKT_END_SESSION;
    p.metadata = "end";
    p.payload  = "";
    p.length   = 0;

    sendPkt(p);
    Packet resp = recvPkt();
    if (resp.type == PKT_RESPONSE) {
        gSession.clear();
        printSuccess(resp.payload);
    } else {
        printError(resp.payload);
    }
}

// ── Main ───────────────────────────────────────────────────
int main() {
    enableAnsi();

    if (!netInit()) {
        std::cerr << "[ERROR] Network init failed.\n";
        return 1;
    }

    clearScreen();
    printDivider('=');
    std::cout << BOLD << MAGENTA
              << "    BoardNova  –  Connecting to server...\n" << RESET;
    printDivider('=');

    if (!connectToServer()) {
        printError("Cannot connect to server at " +
                   std::string(SERVER_IP) + ":" + std::to_string(SERVER_PORT));
        printInfo("Make sure server.exe is running first.");
        netCleanup();
        return 1;
    }
    printSuccess("Connected to BoardNova server!");

    std::string choice;
    while (true) {
        printBanner(gUser, gSession);
        std::getline(std::cin, choice);

        if      (choice == "1") { doLogin();         pause(); }
        else if (choice == "2") { doCreateSession();  pause(); }
        else if (choice == "3") { doJoinSession();    pause(); }
        else if (choice == "4") { doListSessions();   pause(); }
        else if (choice == "5") { doUploadImage();    pause(); }
        else if (choice == "6") { doViewTimeline();   pause(); }
        else if (choice == "7") { doAddTask();        pause(); }
        else if (choice == "8") { doCompleteTask();   pause(); }
        else if (choice == "9") { doEndSession();     pause(); }
        else if (choice == "0") {
            clearScreen();
            std::cout << CYAN << "  Goodbye!\n" << RESET;
            break;
        } else {
            printError("Invalid choice. Please enter 0-9.");
            pause();
        }
    }

    CLOSE_SOCK(gSock);
    netCleanup();
    return 0;
}
