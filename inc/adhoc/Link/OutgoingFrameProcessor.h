#pragma once

#include "LinkFrame.h"
#include <vector>
#include <cstdint>

/**
 * @brief Processes outgoing Link Frames for transmission
 * 
 * Responsibilities:
 *  - Wrap packets from Data Plane into LinkFrame
 *  - Calculate CRC / error detection
 *  - Queue frames for transmission
 *  - Send frames to hardware / radio interface
 */
class OutgoingFrameProcessor {
private:
    std::vector<LinkFrame> txQueue;  // Transmission queue

public:
    OutgoingFrameProcessor() = default;

    // ----------------------
    // Frame creation and processing
    // ----------------------
    LinkFrame createFrame(uint8_t srcMac, uint8_t destMac, uint8_t frameType, const std::vector<uint8_t> &payload);
    void calculateCRC(LinkFrame &frame);                 // Calculate CRC for the frame

    // ----------------------
    // Queue management
    // ----------------------
    void enqueueFrame(const LinkFrame &frame);
    bool hasPendingFrames() const;
    LinkFrame dequeueFrame();

    // ----------------------
    // Transmission
    // ----------------------
    void sendFrame(const LinkFrame &frame);             // Send frame to hardware

    // ----------------------
    // Debug / logging
    // ----------------------
    void debugPrintFrame(const LinkFrame &frame) const;
    void debugPrintQueue() const;
};
