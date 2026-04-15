# BoardNova – Distributed Collaborative Whiteboard System

> **COMP72070 | Section 3 | Group 7**  
> A multi-client C++ TCP whiteboard system with session management, versioned image uploads, task tracking, and a bonus web canvas UI.

---

## 📁 Project Structure

```
/
├── common/
│   └── packet.h            # Packet struct + serialization
├── server/
│   ├── server.cpp          # Main TCP server (multi-client)
│   ├── logger.h            # Thread-safe logger → logs.txt
│   ├── state_machine.h     # Client state machine
│   └── session_manager.h   # Sessions, tasks, image versioning
├── client/
│   └── client.cpp          # Console UI client
├── web/
│   ├── index.html          # Web whiteboard (bonus)
│   ├── style.css
│   └── script.js
├── test/
│   ├── test_cases.txt      # 20 test cases (table)
│   └── test_log.txt        # Test execution log
├── storage/                # Auto-created: versioned uploads
└── logs.txt                # Auto-created: server log file
```

---

## ⚙️ Compile Instructions

> **Requires:** MinGW g++ (Windows) with C++17 support

### Server
```bash
g++ server/server.cpp -o server -lws2_32 -lstdc++fs -std=c++17
```

### Client
```bash
g++ client/client.cpp -o client -lws2_32 -std=c++17
```

---

## 🚀 Run Instructions

**Step 1** — Start the server in one terminal:
```bash
./server
```

**Step 2** — Start one or more clients in separate terminals:
```bash
./client
```

---

## 🖥️ Client Menu

```
══════════════════════════════════════════════
       BoardNova  –  Whiteboard System
══════════════════════════════════════════════
  Logged in as : alice
  Session ID   : session1
──────────────────────────────────────────────
  1.  Login
  2.  Create Session
  3.  Join Session
  4.  List Sessions
  5.  Upload Whiteboard Image
  6.  View Timeline
  7.  Add Task
  8.  Complete Task
  9.  End Session
  0.  Exit
──────────────────────────────────────────────
```

---

## 👥 Default Test Users

| Username | Password |
|----------|----------|
| alice    | pass123  |
| bob      | pass456  |
| carol    | pass789  |
| admin    | admin    |

---

## 🗂️ Server State Machine

```
IDLE → AUTHENTICATED → SESSION_ACTIVE → RECEIVING_IMAGE → VERSIONING → SESSION_ACTIVE
                                      → TASK_UPDATE     → SESSION_ACTIVE
                                      → ARCHIVING       → AUTHENTICATED
```

---

## 📦 Packet Protocol

```cpp
struct Packet {
    int    type;       // PKT_LOGIN, PKT_CREATE_SESSION, etc.
    int    length;
    string metadata;   // filename, command name, etc.
    string payload;    // data content
};
```

Serialized over TCP as: `type|length|metadata|payload\n`

---

## 🌐 Web UI (Bonus)

Open `web/index.html` in a browser. Features:
- 🖊 Pen & eraser tools
- 🎨 Color picker + brush size
- ↩ Undo support
- 💾 Save as PNG

---

## 🧪 Testing

See `test/test_cases.txt` for 20 test cases and `test/test_log.txt` for execution results.

**Result: 20/20 PASS**

---

## 📋 Demo Flow

1. Run `./server`
2. Run `./client` (in a new terminal)
3. **Login**: alice / pass123
4. **Create Session**: menu option 2
5. **Upload Image**: menu option 5 → enter any filename
6. **Add Task**: menu option 7 → "Design wireframe"
7. **Complete Task**: menu option 8 → "Design wireframe"
8. **View Timeline**: menu option 6
9. **End Session**: menu option 9
10. Check `logs.txt` for full audit trail

---

## 📜 License

COMP72070 Course Project — Group 7 — 2026
