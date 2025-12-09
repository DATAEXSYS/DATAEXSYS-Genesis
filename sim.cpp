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

int main(int argc, char* argv[]) {
    int num_processes = -1;
    int duration = -1;
    int link_percentage = -1;

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
        }
    }

    if (num_processes <= 0 || duration <= 0 || link_percentage < 0 || link_percentage > 100) {
        std::cerr << "Usage: " << argv[0] << " --nodes <positive_number> --duration <positive_number> --links <0-100>" << std::endl;
        return 1;
    }

    // --- Directory Setup ---
    const std::string output_dir = "simulation_output";
    std::filesystem::remove_all(output_dir); // Clean up previous run
    std::filesystem::create_directory(output_dir);
    std::cout << "Created directory: " << output_dir << std::endl;

    // --- Port Generation ---
    std::vector<int> ports;
    for (int i = 0; i < num_processes; ++i) {
        ports.push_back(8080 + i);
    }

    // Write all available ports to a file
    std::filesystem::path ports_filepath = output_dir;
    ports_filepath /= "ports.txt";
    std::ofstream ports_outfile(ports_filepath);
    for (int port : ports) {
        ports_outfile << port << std::endl;
    }
    ports_outfile.close();
    std::cout << "List of all available ports written to " << ports_filepath << std::endl;


    // --- Topology Generation (Poisson-based random graph) ---
    std::vector<std::vector<int>> adj_list(num_processes);
    double link_prob = link_percentage / 100.0;
    
    std::mt19937 gen(time(nullptr)); // Seed the random number generator
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
            // Fork failed
            std::cerr << "Failed to fork process " << i + 1 << std::endl;
            for (pid_t child_pid : child_pids) {
                kill(child_pid, SIGKILL);
            }
            return 1;
        } else if (pid == 0) {
            // --- Child Process Logic ---
            pid_t my_pid = getpid();
            int port = ports[i];

            // Create and write to the access table file
            std::filesystem::path filepath = output_dir;
            filepath /= "AccessTablefor_" + std::to_string(my_pid) + ".txt";
            std::ofstream outfile(filepath);
            
            // First line is the node's own port
            outfile << port << std::endl;
            
            // Subsequent lines are the ports of connected neighbors
            for (int neighbor_index : adj_list[i]) {
                outfile << ports[neighbor_index] << std::endl;
            }
            outfile.close();

            std::cout << "Child process " << i + 1 << " (PID: " << my_pid << ") created, assigned port " << port 
                      << ". Access table generated. Will run for " << duration << " seconds." << std::endl;
            
            // Simulate work
            sleep(duration);

            std::cout << "Child process " << i + 1 << " (PID: " << my_pid << ") on port " << port << " is finishing." << std::endl;
            exit(0);
        } else {
            // Parent process
            child_pids.push_back(pid);
        }
    }

    // Parent process waits for all children to complete
    std::cout << "Parent process is waiting for all child processes to finish." << std::endl;
    for (int i = 0; i < num_processes; ++i) {
        int status;
        wait(&status);
    }

    std::cout << "All child processes have finished. Parent process is exiting." << std::endl;

    return 0;
}