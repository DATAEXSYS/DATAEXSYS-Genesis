#pragma once

#include "TransportSegment.h"
#include "TransportFlags.h"
#include <vector>
#include <cstdint>

/**
 * @brief Transport Layer module for processing outgoing segments.
 * 
 * Responsibilities:
 *  - Convert application data into TransportSegments
 *  - Assign sequence numbers, flags (ACK/SYN/FIN)
 *  - Forward segments to Control Plane for routing, encryption, and sending
 */
class OutgoingSegmentProcessor {
private:
    uint16_t nextSeqNum = 0;  // Sequence number generator

public:
    OutgoingSegmentProcessor() = default;

    // ----------------------
    // Segment creation
    // ----------------------
    TransportSegment createSegment(uint8_t srcPort, uint8_t destPort, const std::vector<uint8_t> &payload,
                                   TransportFlags flags = TransportFlags::NONE);

    // ----------------------
    // Optional helpers
    // ----------------------
    void setFlags(TransportSegment &segment, TransportFlags flags);  // Set / update flags
    void incrementSeqNum();                                           // Increment internal sequence number

    // ----------------------
    // Debug / logging
    // ----------------------
    void debugPrintSegment(const TransportSegment &segment) const;

    // ----------------------
    // Forwarding
    // ----------------------
    void sendSegment(const TransportSegment &segment, void *cpProcessor); // Placeholder: pass to CP processor
};
