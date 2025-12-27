#pragma once

#include <cstdint>
#include <vector>
#include "TransportFlags.h"

// Transport Layer segment
struct TransportSegment {
    uint8_t srcPort;                // Source application/port
    uint8_t destPort;               // Destination application/port
    uint16_t seqNum;                // Sequence number
    uint16_t ackNum;                // Acknowledgment number (if ACK)
    TransportFlags flags;           // Transport control flags
    std::vector<uint8_t> payload;  // Application data

    // Constructor
    TransportSegment(uint8_t src, uint8_t dest, std::vector<uint8_t> data,
                     uint16_t seq = 0, uint16_t ack = 0, TransportFlags f = TransportFlags::NONE)
        : srcPort(src), destPort(dest), seqNum(seq), ackNum(ack), flags(f), payload(std::move(data)) {}
};
