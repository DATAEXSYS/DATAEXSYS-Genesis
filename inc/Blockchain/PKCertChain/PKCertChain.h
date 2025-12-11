#pragma once
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <array>
#include <algorithm>
#include "Consensus/PoW.h"      // PowChallenge, GeneratePoWChallenge, SolvePowChallenge
#include "nodeReg.h"             // nodeReg

namespace fs = std::filesystem;

// Represents a block in your blockchain
struct Certificate {
    uint32_t nodeID;                       // Node ID
    std::vector<unsigned char> PKSign;     // Ed25519 public key (signing key)
    std::vector<unsigned char> PubEncKey;  // X25519 public key (encryption key)

    PowChallenge challenge;                // PoW challenge
    uint64_t nonce = 0;                    // PoW solution
};

// Blockchain container
class Blockchain {
public:
    Blockchain(const std::string& logDir = "BlockLogs") {
        dir = fs::current_path() / logDir;
        if (!fs::exists(dir)) fs::create_directory(dir);

        logFile = (dir / "blockchain.log").string();
        if (!fs::exists(logFile)) std::ofstream(logFile).close();
    }

    // Add a new block
    void AddBlock(const Certificate& block) {
        blocks.push_back(block);
        LogBlock(block);
    }

    const std::vector<Certificate>& GetBlocks() const { return blocks; }

    void AddBlockWithPoW(uint32_t nodeID,
                         const std::vector<unsigned char>& pkSign,
                         const std::vector<unsigned char>& pubEncKey,
                         nodeReg* prevBlock = nullptr) {

        Certificate block;
        block.nodeID = nodeID;
        block.PKSign = pkSign;
        block.PubEncKey = pubEncKey;

        // Prepare public key array
        std::array<uint8_t, 32> pubKeyArray{};
        size_t copySize = std::min(pubEncKey.size(), pubKeyArray.size());
        std::copy(pubEncKey.begin(), pubEncKey.begin() + copySize, pubKeyArray.begin());

        std::array<uint8_t, 64> signature{}; // placeholder

        // uint16_t id variable to satisfy reference parameter
        uint16_t shortId = static_cast<uint16_t>(nodeID);
        PowChallenge* challenge = GeneratePoWChallenge(prevBlock, shortId, pubKeyArray, signature);

        // Solve PoW
        uint64_t nonce = SolvePowChallenge(challenge);
        block.challenge = *challenge;
        block.nonce = nonce;

        AddBlock(block);
        delete challenge;
    }

private:
    std::vector<Certificate> blocks;
    fs::path dir;
    std::string logFile;

    void LogBlock(const Certificate& block) {
        std::ofstream out(logFile, std::ios::app);
        if (!out.is_open()) {
            std::cerr << "Failed to open blockchain log file: " << logFile << std::endl;
            return;
        }

        out << "Block ID: " << block.nodeID
            << " | PKSign size: " << block.PKSign.size()
            << " | PubEncKey size: " << block.PubEncKey.size()
            << " | Nonce: " << block.nonce
            << std::endl;
    }
};
