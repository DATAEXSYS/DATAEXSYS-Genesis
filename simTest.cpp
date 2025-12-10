#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include "inc/Node/DSRNode.h"

// Constants matching sim.cpp convention
const int RX_BASE_PORT = 8000;
const int TX_BASE_PORT = 9000;

void create_test_access_table(int node_id, const std::vector<int>& neighbors, const std::string& output_dir) {
    std::filesystem::path filepath = output_dir;
    filepath /= "AccessTable.txt";
    std::ofstream outfile(filepath);
    for (int neighbor_id : neighbors) {
        outfile << neighbor_id << std::endl;
    }
    outfile.close();
}

int main() {
    // --- Setup Test Directory ---
    const std::string output_dir = "test_output";
    if (std::filesystem::exists(output_dir)) {
        std::filesystem::remove_all(output_dir);
    }
    std::filesystem::create_directory(output_dir);
    std::filesystem::create_directory(output_dir + "/DSR");
    
    // --- Define Topology: Chain 0 <-> 1 <-> 2 <-> 3 <-> 4 ---
    int num_processes = 5;
    std::vector<std::vector<int>> adj_list(num_processes);
    
    // 0 <-> 1
    adj_list[0].push_back(1);
    adj_list[1].push_back(0);
    
    // 1 <-> 2
    adj_list[1].push_back(2);
    adj_list[2].push_back(1);

    // 2 <-> 3
    adj_list[2].push_back(3);
    adj_list[3].push_back(2);

    // 3 <-> 4
    adj_list[3].push_back(4);
    adj_list[4].push_back(3);

    std::cout << "Starting simTest with 5 nodes (Chain 0-1-2-3-4)..." << std::endl;

    std::vector<pid_t> child_pids;
    for (int i = 0; i < num_processes; ++i) {
        pid_t pid = fork();

        if (pid < 0) {
            std::cerr << "Fork failed!" << std::endl;
            return 1;
        } else if (pid == 0) {
            // --- Child Process ---
            try {
                int node_id = i;
                int rx_port = RX_BASE_PORT + node_id;
                int tx_port = TX_BASE_PORT + node_id;

                // Configure directories
                std::filesystem::path node_dir = output_dir;
                node_dir /= "DSR";
                node_dir /= "Node_" + std::to_string(node_id);
                std::filesystem::create_directory(node_dir);

                // Create Access Table
                create_test_access_table(node_id, adj_list[i], node_dir.string());

                std::cout << "[Node " << node_id << "] Initialized. RX=" << rx_port << " TX=" << tx_port << std::endl;

                // Initialize Node
                DSRNode node(node_id, rx_port, tx_port);
                node.set_node_dir(node_dir.string());

                // Run Simulation Loop
                auto start_time = std::chrono::steady_clock::now();
                
                const int max_packets_per_dest = 3; // Send 3 packets to each destination
                std::vector<int> destinations;
                for(int i=0; i<num_processes; ++i) {
                    if(i != node_id) destinations.push_back(i);
                }

                // Track state per destination
                std::map<int, int> sent_counts;
                std::map<int, std::chrono::steady_clock::time_point> last_attempt;
                
                for(int d : destinations) {
                    sent_counts[d] = 0;
                    last_attempt[d] = start_time;
                }

                // Increase simulation time significantly for all-to-all traffic
                while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(40)) {
                    node.process_events();

                    // --- All-to-All Traffic Logic ---
                    for (int target : destinations) {
                        if (sent_counts[target] < max_packets_per_dest) {
                            // Rate limit: Don't check/send too often per destination
                            if (std::chrono::steady_clock::now() - last_attempt[target] > std::chrono::milliseconds(2000)) {
                                
                                if (node.has_route(target)) {
                                    std::string msg = "Pkt " + std::to_string(sent_counts[target]) + " from " + std::to_string(node_id) + " to " + std::to_string(target);
                                    std::cout << "[Node " << (int)node_id << "] Sending '" << msg << "'" << std::endl;
                                    node.send_data(target, msg);
                                    sent_counts[target]++;
                                    last_attempt[target] = std::chrono::steady_clock::now();
                                } else {
                                    // Trigger Discovery
                                    // std::cout << "[Node " << (int)node_id << "] Discovering " << target << "..." << std::endl;
                                    node.send_data(target, "Discovery"); // Payload doesn't matter for discovery trigger
                                    last_attempt[target] = std::chrono::steady_clock::now();
                                }
                            }
                        }
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                

                // Save stats at the end of the simulation
                node.save_stats();
                node.save_dsr_routes();

                std::cout << "[Node " << node_id << "] Finished." << std::endl;
                
            } catch (const std::exception& e) {
                 std::cerr << "[Node " << i << "] Error: " << e.what() << std::endl;
                 exit(1);
            }
            exit(0);

        } else {
            child_pids.push_back(pid);
        }
    }

    // Wait for children
    for (int i = 0; i < num_processes; ++i) {
        wait(NULL);
    }
    
    std::cout << "Test Complete. Check test_output/ directory for logs." << std::endl;
    return 0;
}
