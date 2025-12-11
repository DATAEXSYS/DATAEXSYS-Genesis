#pragma once
#include <vector>
#include <cstdint>

typedef struct {
    std::vector<uint8_t> challenge; //32 bytes fixed
    std::vector<uint8_t> difficulty; //2 bytes fixed
} PowChallenge;
