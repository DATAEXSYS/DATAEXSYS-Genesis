#pragma once

#include "adhoc/trans/TransportSegment.h"
#include "RouteCache.h"
#include <vector>
#include <cstdint>


/**
 * @brief Control Plane module for processing outgoing packets in an ad-hoc network.
 * 
 * Responsibilities:
 *  - Handle Transport Layer segments from upper layers
 *  - Generate Control Plane packets (RREQ, RERR, PKCertChainBlock, RouteLogChainBlock)
 *  - Add routing headers and perform optional signing/encryption
 *  - Forward prepared packets to Data Plane
 */
class OutgoingPacketProcessorControl {
private:
    RouteCache &routeCache;  // Reference to routing table / next-hop info

public:
    // Constructor
    explicit OutgoingPacketProcessorControl(RouteCache &rc);

    // ----------------------
    // Transport segment handling
    // ----------------------
    void enqueueSegment(const TransportSegment &segment);
    std::vector<uint8_t> processSegment(const TransportSegment &segment);
    
    void addRoutingHeaders(std::vector<uint8_t> &packet, uint8_t dest);
    void signPacket(std::vector<uint8_t> &packet);
    void encryptPayload(std::vector<uint8_t> &packet);

    void logPacket(const std::vector<uint8_t> &packet) const;
    void debugPrintSegment(const TransportSegment &segment) const;
    void debugPrintPacket(const std::vector<uint8_t> &packet) const;

    // ----------------------
    // Control Plane packet generators
    // ----------------------
    std::vector<uint8_t> generateRREQ(uint8_t dest);                  // Route Request
    std::vector<uint8_t> generateRERR(uint8_t unreachableDest);      // Route Error
    std::vector<uint8_t> generatePKCertChainBlock();                 // Public Key / Certificate Block
    std::vector<uint8_t> generateRouteLogChainBlock();               // Route Log / auditing

    // ----------------------
    // Helper to send packets to Data Plane
    // ----------------------
    void sendPacket(const std::vector<uint8_t> &packet);
};

