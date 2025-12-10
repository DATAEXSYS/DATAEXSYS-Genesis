#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <random>
#include <ctime>
#include <chrono>
#include <thread>
#include "inc/Node/DSRNode.h" // Note: Adjust path if necessary based on your build system's include paths

int main(int argc, char* argv[]) {
    int num_processes = -1;
    int duration = -1;
    int link_percentage = -1;
    int loss_percentage = 0; // Default 0 loss

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--nodes" && i + 1 < argc) {
            try {
                num_processes = std::stoi(argv[++i]);
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid number provided for --nodes." << std::endl;
                return 1;
            }
        } else if (arg == "--duration" && i + 1 < argc) {
            try {
                duration = std::stoi(argv[++i]);
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid number provided for --duration." << std::endl;
                return 1;
            }
        } else if (arg == "--links" && i + 1 < argc) {
            try {
                link_percentage = std::stoi(argv[++i]);
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid number provided for --links." << std::endl;
                return 1;
            }
        } else if (arg == "--loss" && i + 1 < argc) {
            try {
                loss_percentage = std::stoi(argv[++i]);
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid number provided for --loss." << std::endl;
                return 1;
            }
        }
    }

    if (num_processes <= 0 || duration <= 0 || link_percentage < 0 || link_percentage > 100 || loss_percentage < 0 || loss_percentage > 100) {
        std::cerr << "Usage: " << argv[0] << " --nodes <positive_number> --duration <positive_number> --links <0-100> --loss <0-100>" << std::endl;
        return 1;
    }

    // --- Directory Setup ---
    const std::string output_dir = "simulation_output";
    if (std::filesystem::exists(output_dir)) {
        std::filesystem::remove_all(output_dir);
    }
    std::filesystem::create_directory(output_dir);
    std::filesystem::create_directory(output_dir + "/DSR");
    std::cout << "Created directory: " << output_dir << std::endl;

    // --- Port Generation ---
    std::vector<int> ports;
    for (int i = 0; i < num_processes; ++i) {
        ports.push_back(8080 + i);
    }

    std::filesystem::path ports_filepath = output_dir;
    ports_filepath /= "ports.txt";
    std::ofstream ports_outfile(ports_filepath);
    for (int port : ports) {
        ports_outfile << port << std::endl;
    }
    ports_outfile.close();
    std::cout << "List of all available ports written to " << ports_filepath << std::endl;


    // --- Topology Generation (Random Graph) ---
    std::vector<std::vector<int>> adj_list(num_processes);
    double link_prob = link_percentage / 100.0;
    
    std::mt19937 gen(time(nullptr));
    std::uniform_real_distribution<> distrib(0.0, 1.0);

    std::cout << "Generating topology with " << link_percentage << "% link probability..." << std::endl;
    for (int i = 0; i < num_processes; ++i) {
        for (int j = i + 1; j < num_processes; ++j) {
            if (distrib(gen) < link_prob) {
                adj_list[i].push_back(j);
                adj_list[j].push_back(i);
            }
        }
    }

    std::cout << "Parent process (PID: " << getpid() << ") is creating " << num_processes << " child processes." << std::endl;

    std::vector<pid_t> child_pids;
    for (int i = 0; i < num_processes; ++i) {
        pid_t pid = fork();

        if (pid < 0) {
            std::cerr << "Failed to fork process " << i + 1 << std::endl;
            for (pid_t child_pid : child_pids) {
                kill(child_pid, SIGKILL);
            }
            return 1;
        } else if (pid == 0) {
            // --- Child Process Logic ---
            pid_t my_pid = getpid();
            int port = ports[i];
            uint8_t node_id = i;

            // Create Node directory
            std::filesystem::path node_dir = output_dir;
            node_dir /= "DSR";
            node_dir /= "Node_" + std::to_string(node_id);
            std::filesystem::create_directory(node_dir);

            // Create and write to the access table file first, so the node can read it.
            // Keeping access table in Node dir for cleanliness
            std::filesystem::path filepath = node_dir;
            filepath /= "AccessTable.txt";
            std::ofstream outfile(filepath);
            outfile << port << std::endl;
            for (int neighbor_index : adj_list[i]) {
                outfile << ports[neighbor_index] << std::endl;
            }
            outfile.close();

            std::cout << "Child process " << (int)node_id << " (PID: " << my_pid << ") created, assigned port " << port << ". Access table generated." << std::endl;
            
            try {
                DSRNode node(node_id, port, loss_percentage);
                node.set_node_dir(node_dir.string());

                // Let the simulation run for a bit before starting discovery
                std::this_thread::sleep_for(std::chrono::seconds(1));

                // Have node 0 initiate a route discovery for the last node
                if (node.get_node_id() == 0 && num_processes > 1) {
                    node.start_route_discovery(num_processes - 1);
                }

                auto start_time = std::chrono::steady_clock::now();
                int packet_counter = 0;
                while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(duration)) {
                    node.process_events();
                    
                    // Send a data packet every second if we are node 0
                    if (node.get_node_id() == 0 && packet_counter < 5) { // Limit to 5 packets
                         static auto last_send_time = std::chrono::steady_clock::now();
                         if (std::chrono::steady_clock::now() - last_send_time > std::chrono::seconds(1)) {
                             node.send_data(num_processes - 1, "Hello DSR");
                             last_send_time = std::chrono::steady_clock::now();
                             packet_counter++;
                         }
                    }

                    // Sleep for a short duration to prevent busy-waiting
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }

                // Save stats at the end of the simulation
                node.save_stats();
                node.save_dsr_routes();

                std::cout << "Child process " << (int)node_id << " (PID: " << my_pid << ") on port " << port << " is finishing." << std::endl;

            } catch (const std::exception& e) {
                std::cerr << "Error in child process " << (int)node_id << ": " << e.what() << std::endl;
                exit(1);
            }
            
            exit(0);
        } else {
            child_pids.push_back(pid);
        }
    }

    std::cout << "Parent process is waiting for all child processes to finish." << std::endl;
    for (int i = 0; i < num_processes; ++i) {
        int status;
        wait(&status);
    }

    std::cout << "All child processes have finished. Parent process is exiting." << std::endl;

    return 0;
}