#pragma once

#include <vector>
#include <cstdint>
#include <arpa/inet.h> // for htons, htonl
#include <ctime>       // for time_t

// ----------- Runtime Endian Detection -----------

inline bool isLittleEndian() {
    uint16_t x = 1;
    return *reinterpret_cast<uint8_t*>(&x) == 1;
}

// ----------- Endian conversions -----------

// 8-bit BE (no conversion needed)
inline std::vector<uint8_t> BE8(uint8_t num) {
    return {num};
}

// 16-bit BE
inline std::vector<uint8_t> BE16(uint16_t num) {
    if (isLittleEndian()) num = htons(num);
    return { static_cast<uint8_t>(num >> 8), static_cast<uint8_t>(num & 0xFF) };
}

// 32-bit BE
inline std::vector<uint8_t> BE32(uint32_t num) {
    if (isLittleEndian()) num = htonl(num);
    return {
        static_cast<uint8_t>((num >> 24) & 0xFF),
        static_cast<uint8_t>((num >> 16) & 0xFF),
        static_cast<uint8_t>((num >> 8) & 0xFF),
        static_cast<uint8_t>(num & 0xFF)
    };
}

// 64-bit BE helper for htonll
inline uint64_t htonll(uint64_t value) {
    if (isLittleEndian()) {
        return ((uint64_t)htonl(value & 0xFFFFFFFF) << 32) | htonl(value >> 32);
    } else {
        return value;
    }
}

// 64-bit BE
inline std::vector<uint8_t> BE64(uint64_t num) {
    num = htonll(num);
    std::vector<uint8_t> bytes(8);
    for (int i = 0; i < 8; i++) {
        bytes[i] = static_cast<uint8_t>((num >> (56 - i * 8)) & 0xFF);
    }
    return bytes;
}

// time_t BE (assumes time_t <= 64-bit)
inline std::vector<uint8_t> BE_time(time_t t) {
    return BE64(static_cast<uint64_t>(t));
}