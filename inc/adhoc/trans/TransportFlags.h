#pragma once

#include <cstdint>
// these are ket just in case
enum class TransportFlags: uint8_t{
    NONE = 0x00,
    ACK  = 0x01,   // Acknowledgment
    SYN  = 0x02,   // Connection setup
    FIN  = 0x04    // Connection teardown
};
// Enable bitwise operations for TransportFlags if needed
inline TransportFlags operator|(TransportFlags a, TransportFlags b) {
    return static_cast<TransportFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline TransportFlags operator&(TransportFlags a, TransportFlags b) {
    return static_cast<TransportFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}