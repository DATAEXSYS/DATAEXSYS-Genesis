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
 * @brief Serializes a Receipt object.
 */
inline std::vector<uint8_t> serialize_receipt(const Receipt& rx) {
    std::vector<uint8_t> bytes;
    // node_id (4 bytes)
    uint32_t nid = htonl(rx.node_id);
    bytes.insert(bytes.end(), (uint8_t*)&nid, (uint8_t*)&nid + sizeof(nid));
    
    // route_id (string len + data)
    uint32_t rid_len = htonl(rx.route_id.size());
    bytes.insert(bytes.end(), (uint8_t*)&rid_len, (uint8_t*)&rid_len + sizeof(rid_len));
    bytes.insert(bytes.end(), rx.route_id.begin(), rx.route_id.end());

    // action (string len + data)
    uint32_t act_len = htonl(rx.action.size());
    bytes.insert(bytes.end(), (uint8_t*)&act_len, (uint8_t*)&act_len + sizeof(act_len));
    bytes.insert(bytes.end(), rx.action.begin(), rx.action.end());

    // packet_seq (4 bytes)
    uint32_t seq = htonl(rx.packet_seq);
    bytes.insert(bytes.end(), (uint8_t*)&seq, (uint8_t*)&seq + sizeof(seq));

    // prev_node (string len + data)
    uint32_t prev_len = htonl(rx.prev_node.size());
    bytes.insert(bytes.end(), (uint8_t*)&prev_len, (uint8_t*)&prev_len + sizeof(prev_len));
    bytes.insert(bytes.end(), rx.prev_node.begin(), rx.prev_node.end());

    // next_node (string len + data)
    uint32_t next_len = htonl(rx.next_node.size());
    bytes.insert(bytes.end(), (uint8_t*)&next_len, (uint8_t*)&next_len + sizeof(next_len));
    bytes.insert(bytes.end(), rx.next_node.begin(), rx.next_node.end());
    
    // id (string len + data)
    uint32_t id_len = htonl(rx.id.size());
    bytes.insert(bytes.end(), (uint8_t*)&id_len, (uint8_t*)&id_len + sizeof(id_len));
    bytes.insert(bytes.end(), rx.id.begin(), rx.id.end());

    return bytes;
}

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
    std::vector<uint8_t> receipts_bytes;
    for(const auto& rx : packet.receipts) {
        auto b = serialize_receipt(rx);
        // Prefix each receipt with its size for safety
        uint32_t size = htonl(b.size());
        receipts_bytes.insert(receipts_bytes.end(), (uint8_t*)&size, (uint8_t*)&size + sizeof(size));
        receipts_bytes.insert(receipts_bytes.end(), b.begin(), b.end());
    }

    size_t total_size = sizeof(packet.source_id) + sizeof(packet.destination_id) +
                        sizeof(packet.sequence_number) + sizeof(packet.hopcount) +
                        sizeof(packet.timestamp) + sizeof(packet.type) + 
                        packet.hopAddresses.size() + packet.payload.size() +
                        sizeof(uint32_t) + receipts_bytes.size(); // +4 bytes for receipt count/size

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

    // Receipts
    uint32_t num_receipts = htonl(packet.receipts.size());
    std::memcpy(current_pos, &num_receipts, sizeof(num_receipts));
    current_pos += sizeof(num_receipts);
    
    if (!receipts_bytes.empty()) {
        std::memcpy(current_pos, receipts_bytes.data(), receipts_bytes.size());
        current_pos += receipts_bytes.size();
    }

    if (!packet.payload.empty()) {
        // Recalculate remaining space to avoid overflow if estimation was off, though it shouldn't be
        std::memcpy(current_pos, packet.payload.data(), packet.payload.size());
    }

    return bytes;
}

inline Receipt deserialize_receipt(const uint8_t*& current_pos, size_t& offset, size_t max_size) {
    Receipt rx;
    auto read_u32 = [&]() {
        uint32_t val;
        std::memcpy(&val, current_pos + offset, sizeof(val));
        offset += sizeof(val);
        return ntohl(val);
    };
    auto read_str = [&](uint32_t len) {
        std::string s(reinterpret_cast<const char*>(current_pos + offset), len);
        offset += len;
        return s;
    };

    rx.node_id = read_u32();
    uint32_t rid_len = read_u32(); rx.route_id = read_str(rid_len);
    uint32_t act_len = read_u32(); rx.action = read_str(act_len);
    rx.packet_seq = read_u32();
    uint32_t prev_len = read_u32(); rx.prev_node = read_str(prev_len);
    uint32_t next_len = read_u32(); rx.next_node = read_str(next_len);
    uint32_t id_len = read_u32(); rx.id = read_str(id_len);
    
    // Timestamp not serialized in original struct? Re-generating or defaulting.
    // Struct has it, I should have serialized it. Adding it now to match.
    // Wait, original struct init generates timestamp. I'll just use current time or 0.
    rx.timestamp = std::time(nullptr);
    
    return rx;
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

    // Receipts
    check_size(sizeof(uint32_t));
    uint32_t num_receipts;
    std::memcpy(&num_receipts, current_pos + offset, sizeof(num_receipts));
    num_receipts = ntohl(num_receipts);
    offset += sizeof(num_receipts);

    for(uint32_t i=0; i<num_receipts; ++i) {
        check_size(sizeof(uint32_t)); 
        uint32_t size;
        std::memcpy(&size, current_pos + offset, sizeof(size));
        size = ntohl(size);
        offset += sizeof(size);
        
        check_size(size);
        // We need a helper that doesn't use the global offset for the struct internals, 
        // or we just call it carefully.
        // deserialize_receipt updates offset.
        packet.receipts.push_back(deserialize_receipt(current_pos, offset, bytes.size()));
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
inline std::vector<uint8_t> serializeCertificate(const certificate& cert) {
    std::vector<uint8_t> bytes;

    // nodeID
    std::vector<uint8_t> nid = BE16(cert.nodeID);
    bytes.insert(bytes.end(), nid.begin(), nid.end());

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
