#pragma once

#include <cstdint>
#include <vector>

/**
 * @brief Represents raw bits for the hardware layer (Physical Layer)
 * 
 * Contains:
 *  - Bitstream for transmission
 *  - Optional metadata (length, modulation, timestamp)
 */
struct HardwareBits {
    std::vector<uint8_t> bitStream;  // Raw bits (packed as bytes)
    uint32_t length;                  // Number of bits
    uint8_t modulationType;           // Example: 0 = OOK, 1 = FSK, etc.
    uint64_t timestamp;               // Optional timestamp for scheduling

    HardwareBits() : length(0), modulationType(0), timestamp(0) {}
    
    HardwareBits(std::vector<uint8_t> bits, uint8_t modType = 0, uint64_t ts = 0)
        : bitStream(std::move(bits)), length(bits.size() * 8), modulationType(modType), timestamp(ts) {}
};
