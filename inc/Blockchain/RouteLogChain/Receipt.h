#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <ctime>
#include "../../Helper/Hash.h"

struct Receipt {
    uint32_t node_id;
    std::string route_id; // Keeping strict to Python example 'packet['uid']'
    std::string action; // e.g., "DATA_forwarded"
    uint32_t packet_seq;
    std::string prev_node; // Optional context
    std::string next_node; // Optional context
    std::string id; // Receipt ID (Hash)
    uint64_t timestamp;

    Receipt(uint32_t nid, std::string rid, std::string act, uint32_t seq, std::string prev = "", std::string next = "")
        : node_id(nid), route_id(rid), action(act), packet_seq(seq), prev_node(prev), next_node(next) {
        
        timestamp = std::time(nullptr);
        
        // Generate ID: hash(node_id + action + rand)
        // Using timestamp as simplistic rand for deterministic simulation if needed, or proper rand.
        std::stringstream ss;
        ss << node_id << action << timestamp << packet_seq; // Simple unique string
        std::string raw = ss.str();
        std::vector<unsigned char> hash = sha256(std::vector<unsigned char>(raw.begin(), raw.end()));
        
        std::stringstream hex;
        for(auto b : hash) hex << std::hex << std::setw(2) << std::setfill('0') << (int)b;
        id = hex.str();
    }
    
    // Default constructor for serialization
    Receipt() : node_id(0), packet_seq(0), timestamp(0) {}

    std::string ToString() const {
        return id + ":" + std::to_string(node_id) + ":" + action;
    }
};
