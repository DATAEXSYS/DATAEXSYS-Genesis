#pragma once

#include <cstdint>
#include <vector>
#include <ctime>

struct certificate {
    uint16_t nodeID;
    std::vector<uint8_t> publicKey;
    std::vector<uint8_t> signature;
    time_t notBefore;
    time_t notAfter;
};
