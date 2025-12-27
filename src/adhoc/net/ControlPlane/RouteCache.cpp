#include "adhoc/net/ControlPlane/RouteCache.h"

// Constructor
RouteCache::RouteCache() = default;

// Add or update a neighbor
bool RouteCache::addNeighbor(uint8_t dest, uint8_t neighbor) {
    routeTable[dest] = neighbor;
    return true;
}

// Delete a neighbor
bool RouteCache::deleteNeighbor(uint8_t dest) {
    return routeTable.erase(dest) > 0;
}

// Find next-hop neighbor
std::optional<uint8_t> RouteCache::findNeighbor(uint8_t dest) const {
    auto it = routeTable.find(dest);
    if (it != routeTable.end()) {
        return it->second;
    } else {
        return std::nullopt;
    }
}

// Check if route exists
bool RouteCache::hasNeighbor(uint8_t dest) const {
    return routeTable.find(dest) != routeTable.end();
}
