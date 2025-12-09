#pragma once
#include <cstdint>
#include <vector>
class BasicPacket {
public:
    uint16_t source_id; //2 bytes
    uint16_t destination_id; //2 bytes
    uint32_t sequence_number; // 4 bytes
    uint64_t timestamp; // 8 bytes
    std::vector<uint8_t> payload;//1474 bytes

    BasicPacket(uint16_t src, uint16_t dest, uint32_t seq_num, uint64_t time, const std::vector<uint8_t>& data)
        : source_id(src), destination_id(dest), sequence_number(seq_num), timestamp(time), payload(data) {}
};



