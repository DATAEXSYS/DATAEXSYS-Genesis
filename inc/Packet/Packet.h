#pragma once
#include <cstdint>
#include <vector>
#include "PacketType.h"

class Packet {
public:
    PacketType type = PacketType::DATA; // Packet type (e.g., DATA, RREQ, RREP)
    uint8_t source_id;      // 1 byte
    uint8_t destination_id; // 1 byte
    uint32_t sequence_number;// 4 bytes
    uint32_t timestamp;      // 4 bytes
    uint8_t hopcount = 0; //  1 byte
    std::vector<uint8_t> hopAddresses; //variable, typically 1 * hopcount bytes
    std::vector<uint8_t> payload; //variable size but will equal to 1489−1×hopcount bytes

    Packet() = default;

    Packet(uint16_t src, uint16_t dest, uint32_t seq_num, uint64_t time, const std::vector<uint8_t>& data, PacketType pk_type)
        : source_id(src), destination_id(dest), sequence_number(seq_num), timestamp(time), payload(data), type(pk_type) {}
};