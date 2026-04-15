## Compile & Run (Windows MinGW)

### Prerequisites
- MinGW-w64 with g++ supporting C++17
- Run from: `c:\Users\Fenish\Downloads\final proj 4\`

---

### 1. Compile Server
```powershell
g++ server/server.cpp -o server.exe -lws2_32 -std=c++17 -I.
```

### 2. Compile Client
```powershell
g++ client/client.cpp -o client.exe -lws2_32 -std=c++17 -I.
```

### 3. Run Server (Terminal 1)
```powershell
./server
```

### 4. Run Client (Terminal 2, 3, ...)
```powershell
./client
```

---

### Quick Demo Script (PowerShell)
Open **two** PowerShell terminals in your project folder:

**Terminal 1:**
```powershell
./server
```

**Terminal 2:**
```powershell
./client
# Then: Login → alice / pass123
# Create Session → option 2
# Upload → option 5 → type: board.txt
# Add Task → option 7 → type: Design wireframe
# Complete Task → option 8 → type: Design wireframe
# View Timeline → option 6
# End Session → option 9 → yes
```

---

### Web Whiteboard (Bonus)
Just open in your browser:
```
web/index.html
```
No server needed — runs stand-alone.
