# Client Module – Developer Notes

## Overview
The client is a single-file console application (`client.cpp`) using Winsock2 for TCP networking and ANSI escape codes for coloured terminal UI.

## Menu Flow
```
printBanner()
    │
    ├─ 1  doLogin()          → PKT_LOGIN
    ├─ 2  doCreateSession()  → PKT_CREATE_SESSION
    ├─ 3  doJoinSession()    → PKT_JOIN_SESSION
    ├─ 4  doListSessions()   → PKT_LIST_SESSIONS
    ├─ 5  doUploadImage()    → PKT_UPLOAD_IMAGE
    ├─ 6  doViewTimeline()   → PKT_VIEW_TIMELINE
    ├─ 7  doAddTask()        → PKT_ADD_TASK
    ├─ 8  doCompleteTask()   → PKT_COMPLETE_TASK
    ├─ 9  doEndSession()     → PKT_END_SESSION
    └─ 0  Exit
```

## State Tracking (Client-side)
- `gUser`    – currently logged-in username (empty if not logged in)
- `gSession` – currently active session ID (empty if not in a session)

These are displayed in the banner on every menu refresh.

## Colours
Uses ANSI escape codes wrapped in macros:
- `CYAN` / `GREEN` / `YELLOW` / `RED` / `MAGENTA`
- Enabled via `SetConsoleMode` on Windows (VT processing)

## Error Handling
- Every server response is checked for `PKT_RESPONSE` vs `PKT_ERROR`
- On connection failure at startup, a clear message is shown before exit

## File Upload
If the specified file doesn't exist locally, a simulated placeholder string is sent instead, so demos work without real files.
