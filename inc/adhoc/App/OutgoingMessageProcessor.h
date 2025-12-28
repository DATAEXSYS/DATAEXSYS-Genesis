#pragma once

#include "AppMessage.h"
#include <vector>
#include <cstdint>

/**
 * @brief Application Layer processor for outgoing messages
 * 
 * Responsibilities:
 *  - Encrypt large application data (X25519 + AES-GCM)
 *  - Fragment encrypted data into AppMessages (~32 MB each)
 *  - Attach sequence number and IV/Nonce per fragment
 *  - Forward AppMessages to Transport Layer
 */
class OutgoingMessageProcessor {
private:
    uint32_t nextAppSeqNum = 0;  // App-level fragment sequence number

    // ----------------------
    // Helper for encryption
    // ----------------------
    std::vector<uint8_t> encryptFragment(const std::vector<uint8_t> &plaintext,
                                         std::vector<uint8_t> &ivOut);

public:
    OutgoingMessageProcessor() = default;

    /**
     * @brief Split large encrypted application data into AppMessages
     * @param src Source app ID
     * @param dest Destination app ID
     * @param plaintext Large plaintext payload (can be GBs)
     * @return Vector of AppMessages (~32 MB each)
     */
    std::vector<AppMessage> fragmentAndEncrypt(uint8_t src, uint8_t dest,
                                               const std::vector<uint8_t> &plaintext);

    /**
     * @brief Forward AppMessages to Transport Layer
     * @param messages Vector of AppMessages
     * @param transportProcessor Reference to transport layer processor (void* placeholder)
     */
    void sendFragments(const std::vector<AppMessage> &messages, void *transportProcessor);

    /**
     * @brief Debug helpers
     */
    void debugPrintMessage(const AppMessage &msg) const;
    void debugPrintAllMessages(const std::vector<AppMessage> &messages) const;
};
