#pragma once
#include <string>
#include <iostream>
#include "logger.h"

// Server-side states per connected client
enum ClientState {
    STATE_IDLE,
    STATE_AUTHENTICATED,
    STATE_SESSION_ACTIVE,
    STATE_RECEIVING_IMAGE,
    STATE_VERSIONING,
    STATE_TASK_UPDATE,
    STATE_ARCHIVING
};

inline std::string stateToString(ClientState s) {
    switch (s) {
        case STATE_IDLE:             return "IDLE";
        case STATE_AUTHENTICATED:    return "AUTHENTICATED";
        case STATE_SESSION_ACTIVE:   return "SESSION_ACTIVE";
        case STATE_RECEIVING_IMAGE:  return "RECEIVING_IMAGE";
        case STATE_VERSIONING:       return "VERSIONING";
        case STATE_TASK_UPDATE:      return "TASK_UPDATE";
        case STATE_ARCHIVING:        return "ARCHIVING";
        default:                     return "UNKNOWN";
    }
}

// Attempt a state transition; returns true if valid
inline bool transition(ClientState& current, ClientState next, const std::string& clientId = "Client") {
    // Define valid transitions
    bool valid = false;
    switch (current) {
        case STATE_IDLE:
            valid = (next == STATE_AUTHENTICATED);
            break;
        case STATE_AUTHENTICATED:
            valid = (next == STATE_SESSION_ACTIVE || next == STATE_IDLE);
            break;
        case STATE_SESSION_ACTIVE:
            valid = (next == STATE_RECEIVING_IMAGE ||
                     next == STATE_TASK_UPDATE     ||
                     next == STATE_ARCHIVING       ||
                     next == STATE_AUTHENTICATED);
            break;
        case STATE_RECEIVING_IMAGE:
            valid = (next == STATE_VERSIONING);
            break;
        case STATE_VERSIONING:
            valid = (next == STATE_SESSION_ACTIVE);
            break;
        case STATE_TASK_UPDATE:
            valid = (next == STATE_SESSION_ACTIVE);
            break;
        case STATE_ARCHIVING:
            valid = (next == STATE_AUTHENTICATED);
            break;
        default:
            valid = false;
    }

    if (valid) {
        LOG_INFO(clientId + " state: " + stateToString(current) + " --> " + stateToString(next));
        current = next;
    } else {
        LOG_WARN(clientId + " INVALID transition attempt: " +
                 stateToString(current) + " --> " + stateToString(next));
    }
    return valid;
}
