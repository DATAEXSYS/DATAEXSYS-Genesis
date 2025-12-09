#pragma once

#include <unordered_map>
#include <cstdint>

class RouteCache {
public:
    /**
     * @brief Adds or updates a route in the cache.
     * 
     * If a route for the given destination already exists, it will be overwritten
     * with the new next_hop.
     * 
     * @param destination The final destination node ID.
     * @param next_hop The next node ID to route packets to.
     */
    void add_or_update(uint8_t destination, uint8_t next_hop) {
        cache[destination] = next_hop;
    }

    /**
     * @brief Retrieves the next hop for a given destination.
     * 
     * @param destination The final destination node ID.
     * @param next_hop_out A reference to store the retrieved next hop ID.
     * @return true if a route was found for the destination, false otherwise.
     */
    bool get_next_hop(uint8_t destination, uint8_t& next_hop_out) const {
        auto it = cache.find(destination);
        if (it != cache.end()) {
            next_hop_out = it->second;
            return true;
        }
        return false;
    }

    /**
     * @brief Checks if a route for a given destination exists in the cache.
     * 
     * @param destination The destination node ID to check.
     * @return true if a route exists, false otherwise.
     */
    bool has_route(uint8_t destination) const {
        return cache.count(destination) > 0;
    }

    /**
     * @brief Removes a route from the cache.
     * 
     * If no route exists for the destination, this function does nothing.
     * 
     * @param destination The destination node ID of the route to remove.
     */
    void remove(uint8_t destination) {
        cache.erase(destination);
    }

    /**
     * @brief Clears all entries from the route cache.
     */
    void clear() {
        cache.clear();
    }

    /**
     * @brief Gets the current number of routes stored in the cache.
     * 
     * @return The number of entries in the cache.
     */
    size_t size() const {
        return cache.size();
    }

private:
    std::unordered_map<uint8_t, uint8_t> cache;
};
