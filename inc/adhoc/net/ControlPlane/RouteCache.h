#pragma once

#include <unordered_map>
#include <cstdint>
#include <optional>

class RouteCache {
private:
    // destination node -> next-hop neighbor
    std::unordered_map<uint8_t, uint8_t> routeTable;

public:
    RouteCache();

    // Add or update a neighbor mapping
    bool addNeighbor(uint8_t dest, uint8_t neighbor);

    // Delete a neighbor mapping
    bool deleteNeighbor(uint8_t dest);

    // Find next-hop neighbor; returns std::nullopt if not found
    std::optional<uint8_t> findNeighbor(uint8_t dest) const;

    // Check if a route exists
    bool hasNeighbor(uint8_t dest) const;
};
