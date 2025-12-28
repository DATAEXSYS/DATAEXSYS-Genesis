#pragma once

#include <cstdint>
#include <vector>

/**
 * @brief Represents a Link Layer frame
 * 
 * Structure includes:
 *  - Source / destination MAC
 *  - Frame type
 *  - Payload (from Data Plane)
 *  - CRC for error detection
 */
struct LinkFrame {
    uint8_t srcMac;                  // Source MAC address
    uint8_t destMac;                 // Destination MAC address
    uint8_t frameType;               // Frame type (e.g., DATA, CONTROL)
    std::vector<uint8_t> payload;    // Payload data (from Data Plane)
    uint16_t crc;                    // CRC16 or CRC32 for error detection

    // Constructor for convenience
    LinkFrame(uint8_t src, uint8_t dest, uint8_t type, std::vector<uint8_t> data)
        : srcMac(src), destMac(dest), frameType(type), payload(std::move(data)), crc(0) {}
};
