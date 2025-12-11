#pragma once

#include <cstdint>
#include <vector>
#include <ctime>
#include "Certificate.h"

// Placeholder struct for node registration / block
struct nodeReg {
    std::vector<uint8_t> prevHash;
    std::vector<uint8_t> nextHash;
    std::vector<uint8_t> blockHash;
    uint64_t nonce;
    uint16_t difficulty;
    time_t timestamp;
    certificate cert;
};