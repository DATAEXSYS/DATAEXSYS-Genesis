#pragma once
#include <vector>
#include <unordered_map>
#include <set>
#include <iostream>
#include "Block.h"

struct NodeMetrics {
    double trust = 0.8;
    int forward_success = 0;
    int packets_received = 0;
    int wormhole_flags = 0;
    int identity_overlap = 0;
};

class Blockchain {
public:
    std::vector<Block> chain;
    std::unordered_map<uint32_t, NodeMetrics> node_history;
    std::set<uint32_t> blacklisted_nodes;
    std::vector<std::string> attack_detection_log;

    Blockchain() {
        // Genesis Block
        std::vector<Receipt> genesis_txs; 
        // We could add a dummy receipt or just empty
        chain.emplace_back("0", genesis_txs);
    }

    void add_block(const Block& block) {
        if (block.prev_hash != chain.back().hash) {
            std::cerr << "Blockchain Error: Invalid prev_hash. Block rejected." << std::endl;
            return;
        }
        chain.push_back(block);
        
        // Process receipts to update node_history
        for (const auto& rx : block.transactions) {
            process_receipt(rx);
        }
    }

    double get_trust_score(uint32_t node_id) {
        // Ensure default entry exists
         if (node_history.find(node_id) == node_history.end()) {
             node_history[node_id] = NodeMetrics();
         }
        return node_history[node_id].trust;
    }
    
    // Add a receipt locally (Evidence collection before mining block)
    // For this simulation, we might bypass mining and just add blocks periodically / or directly?
    // The user requirement says "node_history stores live metrics for consensus".
    // And "In a real system, we would parse receipts here. For simulation, 'node_history' is updated in real-time by nodes acting as monitors."
    // So we should expose a way to update metrics directly or via receipt processing.
    
    void process_receipt(const Receipt& rx) {
        if (node_history.find(rx.node_id) == node_history.end()) {
            node_history[rx.node_id] = NodeMetrics();
        }
        
        NodeMetrics& metrics = node_history[rx.node_id];

        if (rx.action == "DATA_forwarded") {
            metrics.forward_success++;
            // Simple trust increment
            if (metrics.trust < 1.0) metrics.trust += 0.01;
        } 
        else if (rx.action == "PACKET_RECEIVED") {
             metrics.packets_received++;
        }
        // ... Logic for penalties could go here
    }

    // Helper for simulation to visualize state
    void print_chain_stats() const {
        std::cout << "Blockchain Length: " << chain.size() << std::endl;
        for(auto const& [id, m] : node_history) {
            std::cout << "Node " << id << " Trust: " << m.trust << " Fwd: " << m.forward_success << std::endl;
        }
    }
};
