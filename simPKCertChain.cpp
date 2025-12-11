#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <random>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <chrono>

#include "inc/Blockchain/PKCertChain/PKCertChain.h"
#include "Consensus/PoW.h"  // For SolvePowChallenge

// Simulation configuration
const int NUM_NODES = 5;                       // Total nodes
const int SIM_DURATION_SEC = 20;               // Duration of simulation
const std::string DATA_DIR = "pkcert_data";    // Base data directory

// Generate random "keys" for signing/encryption
std::vector<uint8_t> generateRandomKey(size_t size = 32) {
    std::vector<uint8_t> key(size);
    std::random_device rd;
    for (auto &b : key) b = static_cast<uint8_t>(rd() % 256);
    return key;
}

// Node simulation function
void simulate_node(int node_id, PKCertChain* genesisNode = nullptr) {
    std::string node_dir = DATA_DIR + "/node_" + std::to_string(node_id);
    bool is_genesis = (node_id == 0);

    // Only genesis node is allowed to use node_id 0
    PKCertChain node(node_id, node_dir, true, is_genesis);

    std::vector<uint8_t> PKSign = generateRandomKey();
    std::vector<uint8_t> PKEncrypt = generateRandomKey();

    if (is_genesis) {
        // Genesis node: create genesis block
        node.log_info("I am the genesis node. Creating genesis block...");

        nodeReg genesis = node.create_genesis_block(node_id, PKSign, PKEncrypt);
        if (!node.addBlock(genesis)) {
            node.log_error("Failed to add genesis block!");
            return;
        }

        node.log_info("Genesis block added. Hash: " + node.hex(genesis.blockHash, true));
        node.log_info("Genesis node is running. Waiting for other nodes to register...");

        std::this_thread::sleep_for(std::chrono::seconds(SIM_DURATION_SEC));
    } else {
        // Other nodes: register with genesis node
        if (!genesisNode) {
            node.log_error("No genesis node provided for registration!");
            return;
        }

        node.log_info("Registering with genesis node...");

        try {
            std::vector<uint8_t> nodeIdVec = { static_cast<uint8_t>(node_id) };
            PowChallenge chal = genesisNode->addNode(nodeIdVec, PKSign, PKEncrypt);

            if (chal.challenge.empty()) {
                node.log_error("Failed to get PoW challenge from genesis node");
                return;
            }

            node.log_info("Received PoW challenge. Solving...");
            uint64_t nonce = SolvePowChallenge(chal);

            bool success = genesisNode->finalizeNode(nodeIdVec, PKSign, PKEncrypt, chal, nonce);
            if (!success) {
                node.log_error("Node registration failed.");
                return;
            }

            // Log the latest block
            const nodeReg* latest_block = genesisNode->getBlockPtr(genesisNode->size() - 1);
            if (latest_block) {
                node.log_info("Latest block hash: " + genesisNode->hex(latest_block->blockHash, true));
            }

            node.log_info("Node registration finalized successfully.");
            node.log_info("Current chain state: " + genesisNode->get_chain_state());

        } catch (const std::exception& e) {
            node.log_error(std::string("Error during node registration: ") + e.what());
        }
    }
}

int main() {
    // Create base data directory
    std::filesystem::create_directories(DATA_DIR);

    // Create genesis node
    PKCertChain genesisNode(0, DATA_DIR + "/node_0", true, true);

    std::vector<std::thread> threads;

    // Start genesis node thread
    threads.emplace_back([&genesisNode]() {
        simulate_node(0, &genesisNode);
    });

    std::this_thread::sleep_for(std::chrono::seconds(1)); // Ensure genesis node is initialized

    // Start other nodes
    for (int i = 1; i < NUM_NODES; ++i) {
        threads.emplace_back([i, &genesisNode]() {
            simulate_node(i, &genesisNode);
        });
    }

    // Wait for all threads to finish
    for (auto& t : threads) {
        t.join();
    }

    std::cout << "Simulation complete. Blockchain contains "
              << genesisNode.size() << " blocks." << std::endl;

    return 0;
}
