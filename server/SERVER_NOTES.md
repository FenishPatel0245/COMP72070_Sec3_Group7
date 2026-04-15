# Server Module – Developer Notes

## Architecture
The server uses a **thread-per-client** model. Each accepted connection spawns a detached `std::thread` running `handleClient()`.

## State Machine
Each client connection maintains its own `ClientState` enum variable. State transitions are validated via the `transition()` function in `state_machine.h`.

```
IDLE
 └─[LOGIN ok]──────────────► AUTHENTICATED
                               └─[CREATE/JOIN]──► SESSION_ACTIVE
                                                   ├─[UPLOAD]──► RECEIVING_IMAGE ──► VERSIONING ──► SESSION_ACTIVE
                                                   ├─[TASK]────► TASK_UPDATE ─────────────────────► SESSION_ACTIVE
                                                   └─[END]─────► ARCHIVING ───────────────────────► AUTHENTICATED
```

## Packet Routing
All packet types are handled in a switch-case in `handleClient()`. Unknown packet types return a PKT_ERROR.

## Thread Safety
- `SessionManager` uses a `std::mutex` for all session and task operations.
- `Logger` uses a separate `std::mutex` for log file writes.

## Storage
Uploaded files are saved to `./storage/` relative to the server executable's working directory.

## Logging
All events are written to `logs.txt` with format:
```
[YYYY-MM-DD HH:MM:SS][LEVEL] message
```
