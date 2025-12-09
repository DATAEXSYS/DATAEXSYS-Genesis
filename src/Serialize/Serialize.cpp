#include "../../inc/Serialize/Serialize.h"

inline std::vector<uint8_t> serialize_packet(const Packet &packet) {
  size_t total_size = sizeof(packet.source_id) + sizeof(packet.destination_id) +
                      sizeof(packet.sequence_number) +
                      sizeof(packet.timestamp) + packet.payload.size();

  std::vector<uint8_t> bytes(total_size);

  // Use a pointer to traverse the byte vector
  char *current_pos = reinterpret_cast<char *>(bytes.data());

  // Copy the fixed-size fields into the byte vector
  std::memcpy(current_pos, &packet.source_id, sizeof(packet.source_id));
  current_pos += sizeof(packet.source_id);

  std::memcpy(current_pos, &packet.destination_id,
              sizeof(packet.destination_id));
  current_pos += sizeof(packet.destination_id);

  std::memcpy(current_pos, &packet.sequence_number,
              sizeof(packet.sequence_number));
  current_pos += sizeof(packet.sequence_number);

  std::memcpy(current_pos, &packet.timestamp, sizeof(packet.timestamp));
  current_pos += sizeof(packet.timestamp);

  // Copy the variable-size payload into the byte vector
  if (!packet.payload.empty()) {
    std::memcpy(current_pos, packet.payload.data(), packet.payload.size());
  }

  return bytes;
}