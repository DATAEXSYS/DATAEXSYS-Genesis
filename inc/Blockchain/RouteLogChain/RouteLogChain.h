#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <functional>   // <- required for std::function
#include "Trust/LocalTrustDiary.h"

namespace fs = std::filesystem;

struct RouteBlock {
    uint32_t source;
    uint32_t destination;
    std::vector<uint32_t> hopVector;
    double score;      // shortest hop + highest trust
    uint64_t timestamp;
};

class RouteLogChain {
private:
    std::vector<RouteBlock> chain;
    fs::path logDir;
    std::string logFile;
public:
    RouteLogChain(const std::string& dirName = "RouteLogs") {
        logDir = fs::current_path() / dirName;
        if (!fs::exists(logDir)) fs::create_directories(logDir);
        logFile = (logDir / "route_chain.log").string();
        if (!fs::exists(logFile)) std::ofstream(logFile).close();
    }

    void addRouteBlock(const RouteBlock& block) {
        chain.push_back(block);
        logBlock(block);
    }

    const std::vector<RouteBlock>& getChain() const { return chain; }

    void logBlock(const RouteBlock& block) {
        std::ofstream out(logFile, std::ios::app);
        if (!out.is_open()) {
            std::cerr << "Failed to open RouteLogChain log: " << logFile << std::endl;
            return;
        }
        out << "Source: " << block.source
            << " | Dest: " << block.destination
            << " | Hops: ";
        for (auto h : block.hopVector) out << h << " ";
        out << "| Score: " << block.score
            << " | Time: " << block.timestamp
            << std::endl;
    }
};

// ----------------------------
// Helper functions
// ----------------------------
inline std::vector<std::vector<uint32_t>> simulateRREQ(uint32_t source,
                                                       uint32_t destination,
                                                       const std::vector<std::vector<uint32_t>>& network,
                                                       size_t maxHops = 5) {
    std::vector<std::vector<uint32_t>> routes;
    std::vector<uint32_t> path;
    std::function<void(uint32_t, size_t)> dfs = [&](uint32_t node, size_t depth) {
        if (depth > maxHops) return;
        path.push_back(node);
        if (node == destination) {
            routes.push_back(path);
        } else {
            for (auto neighbor : network[node]) {
                if (std::find(path.begin(), path.end(), neighbor) == path.end()) {
                    dfs(neighbor, depth + 1);
                }
            }
        }
        path.pop_back();
    };
    dfs(source, 0);
    return routes;
}

inline double calculateRouteScore(const std::vector<uint32_t>& hopVector,
                                  LocalTrustDiary& trustDiary) {
    size_t hopCount = hopVector.size();
    double highestTrust = 0.0;
    for (auto node : hopVector) {
        double t = trustDiary.getTrust(node);
        if (t > highestTrust) highestTrust = t;
    }
    return (1.0 / hopCount) + highestTrust;
}

inline void buildRouteBlocks(uint32_t source,
                             uint32_t destination,
                             const std::vector<std::vector<uint32_t>>& paths,
                             LocalTrustDiary& trustDiary,
                             RouteLogChain& chain) {
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    for (const auto& path : paths) {
        RouteBlock block{};
        block.source = source;
        block.destination = destination;
        block.hopVector = path;
        block.score = calculateRouteScore(path, trustDiary);
        block.timestamp = timestamp;
        chain.addRouteBlock(block);
    }
}
