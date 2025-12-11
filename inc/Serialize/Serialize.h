#pragma once

#include "Packet/Packet.h"
#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <array>
#include <ctime>
#include <arpa/inet.h> // for htons, htonl
#include <Blockchain/PKCertChain/Certificate.h>
#include <Blockchain/PKCertChain/nodeReg.h>
#include <Consensus/PoWchallenge.h>

/**
 * @brief Serializes a Packet object into a byte vector.
 * 
 * This function takes a Packet object and converts it into a flat byte vector
 * for transmission or storage. It handles both fixed-size headers and the
 * variable-size payload.
 * 
 * @param packet The Packet object to serialize.
 * @return A std::vector<uint8_t> containing the serialized bytes of the packet.
 */
inline std::vector<uint8_t> serialize_packet(const Packet &packet) {
    size_t total_size = sizeof(packet.source_id) + sizeof(packet.destination_id) +
                        sizeof(packet.sequence_number) + sizeof(packet.hopcount) +
                        sizeof(packet.timestamp) + sizeof(packet.type) + packet.hopAddresses.size() + packet.payload.size();

    std::vector<uint8_t> bytes(total_size);

    char *current_pos = reinterpret_cast<char *>(bytes.data());
    
    // Serialize Packet Type
    std::memcpy(current_pos, &packet.type, sizeof(packet.type));
    current_pos += sizeof(packet.type);

    std::memcpy(current_pos, &packet.source_id, sizeof(packet.source_id));
    current_pos += sizeof(packet.source_id);

    std::memcpy(current_pos, &packet.destination_id, sizeof(packet.destination_id));
    current_pos += sizeof(packet.destination_id);

    std::memcpy(current_pos, &packet.sequence_number, sizeof(packet.sequence_number));
    current_pos += sizeof(packet.sequence_number);

    std::memcpy(current_pos, &packet.timestamp, sizeof(packet.timestamp));
    current_pos += sizeof(packet.timestamp);

    std::memcpy(current_pos, &packet.hopcount, sizeof(packet.hopcount));
    current_pos += sizeof(packet.hopcount);

    if(!packet.hopAddresses.empty()) {
        std::memcpy(current_pos, packet.hopAddresses.data(), packet.hopAddresses.size());
        current_pos += packet.hopAddresses.size();
    }

    if (!packet.payload.empty()) {
        std::memcpy(current_pos, packet.payload.data(), packet.payload.size());
    }

    return bytes;
}

inline Packet deserialize_packet(const std::vector<uint8_t>& bytes) {
    Packet packet;
    const uint8_t* current_pos = bytes.data();
    size_t offset = 0;

    auto check_size = [&](size_t needed) {
        if (offset + needed > bytes.size()) {
            throw std::runtime_error("Unexpected end of byte stream during deserialization.");
        }
    };
    
    // Deserialize Packet Type
    check_size(sizeof(packet.type));
    std::memcpy(&packet.type, current_pos + offset, sizeof(packet.type));
    offset += sizeof(packet.type);
    
    check_size(sizeof(packet.source_id));
    std::memcpy(&packet.source_id, current_pos + offset, sizeof(packet.source_id));
    offset += sizeof(packet.source_id);

    check_size(sizeof(packet.destination_id));
    std::memcpy(&packet.destination_id, current_pos + offset, sizeof(packet.destination_id));
    offset += sizeof(packet.destination_id);

    check_size(sizeof(packet.sequence_number));
    std::memcpy(&packet.sequence_number, current_pos + offset, sizeof(packet.sequence_number));
    offset += sizeof(packet.sequence_number);
    
    check_size(sizeof(packet.timestamp));
    std::memcpy(&packet.timestamp, current_pos + offset, sizeof(packet.timestamp));
    offset += sizeof(packet.timestamp);
    
    check_size(sizeof(packet.hopcount));
    std::memcpy(&packet.hopcount, current_pos + offset, sizeof(packet.hopcount));
    offset += sizeof(packet.hopcount);

    if (packet.hopcount > 0) {
        check_size(packet.hopcount);
        packet.hopAddresses.assign(current_pos + offset, current_pos + offset + packet.hopcount);
        offset += packet.hopcount;
    }

    size_t payload_size = bytes.size() - offset;
    if (payload_size > 0) {
        packet.payload.assign(current_pos + offset, current_pos + offset + payload_size);
    }
    
    return packet;
}

#include "Helper/BigEndian.h"

// ----------- Serialization -----------

// Serialize a certificate
// Serialize a certificate
inline std::vector<uint8_t> serializeCertificate(const certificate& cert) {
    std::vector<uint8_t> bytes;

    // nodeID (now uint8_t)
    bytes.push_back(cert.nodeID);  // Directly push the uint8_t

    // publicKey
    bytes.insert(bytes.end(), cert.publicKey.begin(), cert.publicKey.end());

    // signature
    bytes.insert(bytes.end(), cert.signature.begin(), cert.signature.end());

    // notBefore, notAfter
    std::vector<uint8_t> nb = BE_time(cert.notBefore);
    std::vector<uint8_t> na = BE_time(cert.notAfter);
    bytes.insert(bytes.end(), nb.begin(), nb.end());
    bytes.insert(bytes.end(), na.begin(), na.end());

    return bytes;
}

// Serialize a nodeReg (excluding pointers)
inline std::vector<uint8_t> serializeNode(const nodeReg& node) {
    std::vector<uint8_t> bytes;

    // prevHash, nextHash, blockHash
    bytes.insert(bytes.end(), node.prevHash.begin(), node.prevHash.end());
    bytes.insert(bytes.end(), node.nextHash.begin(), node.nextHash.end());
    bytes.insert(bytes.end(), node.blockHash.begin(), node.blockHash.end());

    // nonce (serialize length first)
    uint64_t nonceSize = node.nonce.size();
    std::vector<uint8_t> nonceSizeBE = BE64(nonceSize);
    bytes.insert(bytes.end(), nonceSizeBE.begin(), nonceSizeBE.end());
    bytes.insert(bytes.end(), node.nonce.begin(), node.nonce.end());

    // difficulty
    std::vector<uint8_t> diff = BE16(node.difficulty);
    bytes.insert(bytes.end(), diff.begin(), diff.end());

    // timestamp
    std::vector<uint8_t> ts = BE_time(node.timestamp);
    bytes.insert(bytes.end(), ts.begin(), ts.end());

    // certificate
    std::vector<uint8_t> certBytes = serializeCertificate(node.cert);
    bytes.insert(bytes.end(), certBytes.begin(), certBytes.end());

    return bytes;
}

inline std::vector<uint8_t> serializePowChallenge(const PowChallenge& chal) {
    std::vector<uint8_t> bytes;
    
    // Append R (32 bytes)
    bytes.insert(bytes.end(), chal.challenge.begin(), chal.challenge.end());

    // Append T (2 bytes)
    bytes.insert(bytes.end(), chal.difficulty.begin(), chal.difficulty.end());

    return bytes;
}
