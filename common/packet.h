#pragma once
#include <string>
#include <sstream>

// Packet type identifiers
enum PacketType {
    PKT_LOGIN           = 1,
    PKT_CREATE_SESSION  = 2,
    PKT_JOIN_SESSION    = 3,
    PKT_UPLOAD_IMAGE    = 4,
    PKT_VIEW_TIMELINE   = 5,
    PKT_ADD_TASK        = 6,
    PKT_COMPLETE_TASK   = 7,
    PKT_END_SESSION     = 8,
    PKT_RESPONSE        = 9,
    PKT_ERROR           = 10,
    PKT_LIST_SESSIONS   = 11
};

// Core packet structure
struct Packet {
    int    type;
    int    length;
    std::string metadata;
    std::string payload;
};

// Serialize: "type|length|metadata|payload\n"
inline std::string serializePacket(const Packet& p) {
    std::ostringstream oss;
    std::string body = p.metadata + "|" + p.payload;
    oss << p.type << "|" << body.size() << "|" << body << "\n";
    return oss.str();
}

// Deserialize from raw string
inline Packet deserializePacket(const std::string& raw) {
    Packet p;
    std::istringstream iss(raw);
    std::string token;

    std::getline(iss, token, '|'); p.type   = std::stoi(token);
    std::getline(iss, token, '|'); p.length = std::stoi(token);
    std::getline(iss, p.metadata, '|');
    std::getline(iss, p.payload);
    // strip trailing newline
    if (!p.payload.empty() && p.payload.back() == '\n')
        p.payload.pop_back();
    return p;
}

// Helper: build a response packet
inline Packet makeResponse(const std::string& msg) {
    Packet p;
    p.type     = PKT_RESPONSE;
    p.metadata = "OK";
    p.payload  = msg;
    p.length   = (int)(p.metadata.size() + 1 + p.payload.size());
    return p;
}

// Helper: build an error packet
inline Packet makeError(const std::string& msg) {
    Packet p;
    p.type     = PKT_ERROR;
    p.metadata = "ERR";
    p.payload  = msg;
    p.length   = (int)(p.metadata.size() + 1 + p.payload.size());
    return p;
}
