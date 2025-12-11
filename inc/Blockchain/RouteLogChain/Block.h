#pragma once
#include <string>
#include <vector>
#include "Receipt.h"
#include "../../Helper/Hash.h"

struct Block {
    std::string prev_hash;
    std::vector<Receipt> transactions; // In this context, transactions are receipts
    uint64_t timestamp;
    std::string hash;

    Block(std::string prev, std::vector<Receipt> txs) 
        : prev_hash(prev), transactions(txs) {
        timestamp = std::time(nullptr);
        calculate_hash();
    }

    void calculate_hash() {
        std::stringstream ss;
        ss << prev_hash;
        for(const auto& rx : transactions) {
            ss << rx.id;
        }
        ss << timestamp;
        
        std::string raw = ss.str();
        std::vector<unsigned char> h = sha256(std::vector<unsigned char>(raw.begin(), raw.end()));
        
        std::stringstream hex;
        for(auto b : h) hex << std::hex << std::setw(2) << std::setfill('0') << (int)b;
        hash = hex.str();
    }
};
