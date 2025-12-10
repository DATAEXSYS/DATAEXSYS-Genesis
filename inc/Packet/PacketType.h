#pragma once

#include <cstdint>

enum class PacketType : uint8_t {
    DATA,
    RREQ, // Route Request
    RREP, // Route Reply
    RERR, // Route Error
    ACK,  // Acknowledgment
    IDENTITY // Identity assignment
};
