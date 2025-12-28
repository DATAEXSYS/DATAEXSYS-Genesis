#pragma once

#include <cstdint>
#include <vector>

/**
 * @brief Represents a single application-level fragment
 * 
 * Each AppMessage:
 *  - Has a payload of ~32 MB max (32,767,500 bytes)
 *  - Carries IV/Nonce for AES-GCM encryption
 *  - Has a sequence number for reassembly at the receiver
 */
struct AppMessage {
    uint8_t srcApp;                  // Source application ID / port
    uint8_t destApp;                 // Destination application ID / port
    uint32_t seqNum;                 // Sequence number for the app fragment
    std::vector<uint8_t> payload;    // Encrypted payload (~32MB max)
    std::vector<uint8_t> iv;         // IV / Nonce for AES-GCM (12-16 bytes typical)

    AppMessage(uint8_t src, uint8_t dest, uint32_t seq,
               std::vector<uint8_t> data, std::vector<uint8_t> ivData)
        : srcApp(src), destApp(dest), seqNum(seq),
          payload(std::move(data)), iv(std::move(ivData)) {}
};
