#pragma once

#include "Packet/Packet.h"
#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>

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

    // --- Correct final size computation ---
    size_t total_size =
        sizeof(uint8_t) +                 // type (encoded as uint8_t)
        sizeof(packet.source_id) +
        sizeof(packet.destination_id) +
        sizeof(packet.sequence_number) +
        sizeof(packet.timestamp) +
        sizeof(packet.hopcount) +
        packet.hopAddresses.size() +
        packet.payload.size();

    std::vector<uint8_t> bytes(total_size);
    char *current_pos = reinterpret_cast<char *>(bytes.data());

    // Serialize Packet Type as uint8_t
    uint8_t type_byte = static_cast<uint8_t>(packet.type);
    std::memcpy(current_pos, &type_byte, sizeof(type_byte));
    current_pos += sizeof(type_byte);

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

    if (!packet.hopAddresses.empty()) {
        std::memcpy(current_pos, packet.hopAddresses.data(), packet.hopAddresses.size());
        current_pos += packet.hopAddresses.size();
    }

    if (!packet.payload.empty()) {
        std::memcpy(current_pos, packet.payload.data(), packet.payload.size());
    }

    return bytes;
}

/**
 * @brief Deserializes a byte vector into a Packet object.
 *
 * This function takes a byte vector and converts it back into a Packet object.
 *
 * @param bytes The byte vector to deserialize.
 * @return Packet The reconstructed Packet object.
 */
inline Packet deserialize_packet(const std::vector<uint8_t>& bytes) {
    Packet packet;
    const uint8_t* current_pos = bytes.data();
    size_t offset = 0;

    auto check_size = [&](size_t needed) {
        if (offset + needed > bytes.size()) {
            throw std::runtime_error("Unexpected end of byte stream during deserialization.");
        }
    };

    // Deserialize Packet Type (stored as uint8_t)
    check_size(sizeof(uint8_t));
    uint8_t type_byte;
    std::memcpy(&type_byte, current_pos + offset, sizeof(type_byte));
    packet.type = static_cast<PacketType>(type_byte);
    offset += sizeof(type_byte);

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
        packet.hopAddresses.assign(current_pos + offset,
                                   current_pos + offset + packet.hopcount);
        offset += packet.hopcount;
    }

    size_t payload_size = bytes.size() - offset;
    if (payload_size > 0) {
        packet.payload.assign(current_pos + offset,
                              current_pos + offset + payload_size);
    }

    return packet;
}
