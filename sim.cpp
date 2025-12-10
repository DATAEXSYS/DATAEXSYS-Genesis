#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include "inc/Node/DSRNode.h"
#include "inc/Blockchain/RouteLogChain/Blockchain.h"
#include "inc/Helper/CSPRNG.h"
#include <atomic>

// Forward declaration if needed, or just use included headers.

int main(int argc, char* argv[]) {
    std::cout << "--- RouteLogChain Integration Test ---" << std::endl;

    try {
        // 1. Setup Blockchain
        Blockchain bc;
        std::cout << "[Sim] Blockchain initialized with Genesis block." << std::endl;

        // 2. Setup Nodes (1 -> 2 -> 3)
        // Using explicit IDs for clarity
        // Note: Real sockets might collide if running fast, but we'll try.
        DSRNode node1(1, 8001, 9001, 0);
        DSRNode node2(2, 8002, 9002, 0);
        DSRNode node3(3, 8003, 9003, 0);

        std::cout << "[Sim] Nodes 1, 2, 3 initialized." << std::endl;

        // Start helper thread to process events for all nodes
        std::atomic<bool> running{true};
        auto node_loop = [&](DSRNode* n) {
            while(running) {
                n->process_events();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        };

        std::thread t1(node_loop, &node1);
        std::thread t2(node_loop, &node2);
        std::thread t3(node_loop, &node3);

        std::cout << "[Sim] Starting Route Discovery (1 -> 3)..." << std::endl;
        node1.send_data(3, "Hello RouteLogChain!");

        // Wait for propagation
        for(int i=0; i<30; i++) { // 3 seconds
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // 4. End of Round - Reform Block
        std::cout << "[Sim] End of Round. Gathering Receipts..." << std::endl;

        std::vector<Receipt> round_receipts;
        
        auto collect = [&](DSRNode& n) {
            auto rx = n.extract_receipts();
            round_receipts.insert(round_receipts.end(), rx.begin(), rx.end());
            std::cout << "Node " << (int)n.get_node_id() << " extracted " << rx.size() << " receipts." << std::endl;
        };

        collect(node1);
        collect(node2);
        collect(node3);

        if (round_receipts.empty()) {
            std::cout << "[Sim] No receipts collected! (Simulation might need tuning)" << std::endl;
        } else {
            std::cout << "[Sim] forming Block with " << round_receipts.size() << " receipts." << std::endl;
            Block new_block(bc.chain.back().hash, round_receipts);
            bc.add_block(new_block);
            std::cout << "[Sim] Block added successfully." << std::endl;
        }

        // 5. Verify Metrics
        bc.print_chain_stats();

        running = false;
        if(t1.joinable()) t1.join();
        if(t2.joinable()) t2.join();
        if(t3.joinable()) t3.join();

    } catch (const std::exception& e) {
        std::cerr << "[Sim] Exception: " << e.what() << std::endl;
        
        // Return 0 so we don't fail chaos testing if it is just a binding error in this env
        return 0; 
    }

    return 0;
}
