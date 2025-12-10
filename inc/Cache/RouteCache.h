#pragma once

#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <string>

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
     * @brief Removes all routes that use a specific link.
     * 
     * In a full implementation, the cache would store full paths.
     * Since this simple cache only stores (destination -> next_hop),
     * we will remove any route where the next_hop matches the failed link's target.
     * 
     * @param next_hop The node ID of the failed link.
     */
    void remove_routes_with_next_hop(uint8_t next_hop) {
        auto it = cache.begin();
        while (it != cache.end()) {
            if (it->second == next_hop) {
                it = cache.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * @brief Gets the current number of routes stored in the cache.
     * 
     * @return The number of entries in the cache.
     */
    size_t size() const {
        return cache.size();
    }

    /**
     * @brief Saves the route cache to a text file.
     * @param filepath The path to the file where the cache will be saved.
     */
    void save_to_file(const std::string& filepath) const {
        std::ofstream outfile(filepath);
        if (!outfile.is_open()) {
            return;
        }
        outfile << "Destination -> Next Hop\n";
        outfile << "-----------------------\n";
        for (const auto& pair : cache) {
            outfile << (int)pair.first << " -> " << (int)pair.second << "\n";
        }
        outfile.close();
    }

private:
    std::unordered_map<uint8_t, uint8_t> cache;
};
