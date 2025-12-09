#pragma once
#include <cstdint>

struct BasicPacket {
    uint8_t type;
    uint16_t length;
};