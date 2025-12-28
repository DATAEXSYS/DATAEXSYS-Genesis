#pragma once

#include "HardwareBits.h"
#include "adhoc/Link/LinkFrame.h"
#include <vector>
#include <cstdint>

/**
 * @brief Converts Link Frames into raw bits for hardware transmission
 * 
 * Responsibilities:
 *  - Encode LinkFrame into raw bits (serialization, FEC, etc.)
 *  - Queue bits for transmission
 *  - Send to hardware interface / radio
 */
class OutgoingBitsProcessor {
private:
    std::vector<HardwareBits> txQueue;  // Queue of bits for transmission

public:
    OutgoingBitsProcessor() = default;

    // ----------------------
    // Bitstream creation
    // ----------------------
    HardwareBits frameToBits(const LinkFrame &frame);  // Convert LinkFrame to raw bits
    void encodeBits(HardwareBits &bits);              // Optional: FEC, scrambling, etc.

    // ----------------------
    // Queue management
    // ----------------------
    void enqueueBits(const HardwareBits &bits);
    bool hasPendingBits() const;
    HardwareBits dequeueBits();

    // ----------------------
    // Transmission
    // ----------------------
    void sendBits(const HardwareBits &bits);           // Send to actual hardware / radio

    // ----------------------
    // Debug / logging
    // ----------------------
    void debugPrintBits(const HardwareBits &bits) const;
    void debugPrintQueue() const;
};
