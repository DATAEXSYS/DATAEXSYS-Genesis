#pragma once

#include <vector>
#include <cstdint>

/**
 * @brief Data Plane module for sending outgoing packets to the Link/Hardware layer.
 * 
 * Responsibilities:
 *  - Take packets prepared by the Control Plane
 *  - Queue packets for transmission
 *  - Handle retries / retransmissions if needed
 *  - Forward to Link Layer / hardware interface
 */
class OutgoingPacketProcessorData {
private:
    // Optional internal queue for outgoing packets
    std::vector<std::vector<uint8_t>> packetQueue;

public:
    // Constructor
    OutgoingPacketProcessorData() = default;

    // ----------------------
    // Packet queue management
    // ----------------------
    void enqueuePacket(const std::vector<uint8_t> &packet);  // Add packet to queue
    bool hasPendingPackets() const;                          // Check if queue is non-empty
    std::vector<uint8_t> dequeuePacket();                    // Pop packet for transmission

    // ----------------------
    // Transmission helpers
    // ----------------------
    void sendPacket(const std::vector<uint8_t> &packet);     // Send packet to Link Layer / Hardware
    void retryPacket(const std::vector<uint8_t> &packet);   // Retry transmission if failed

    // ----------------------
    // Debug / logging
    // ----------------------
    void debugPrintQueue() const;
    void debugPrintPacket(const std::vector<uint8_t> &packet) const;
};
