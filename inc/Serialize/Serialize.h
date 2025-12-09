#pragma once

#include "../Packet/Packet.h"
#include <vector>
#include <cstdint>
#include <cstring>

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
inline std::vector<uint8_t> serialize_packet(const Packet& packet);
