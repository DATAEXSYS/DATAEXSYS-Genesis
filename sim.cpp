#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>

int main(int argc, char* argv[]) {
    int num_processes = -1;
    int duration = -1;

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
        }
    }

    if (num_processes <= 0 || duration <= 0) {
        std::cerr << "Usage: " << argv[0] << " --nodes <positive_number> --duration <positive_number>" << std::endl;
        return 1;
    }

    std::cout << "Parent process (PID: " << getpid() << ") is creating " << num_processes << " child processes." << std::endl;

    std::vector<pid_t> child_pids;
    for (int i = 0; i < num_processes; ++i) {
        pid_t pid = fork();

        if (pid < 0) {
            // Fork failed
            std::cerr << "Failed to fork process " << i + 1 << std::endl;
            // Terminate already created children before exiting
            for (pid_t child_pid : child_pids) {
                kill(child_pid, SIGKILL);
            }
            return 1;
        } else if (pid == 0) {
            // Child process
            std::cout << "Child process " << i + 1 << " (PID: " << getpid() << ") created, will run for " << duration << " seconds." << std::endl;
            // Simulate work
            sleep(duration);
            std::cout << "Child process " << i + 1 << " (PID: " << getpid() << ") is finishing." << std::endl;
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
        pid_t finished_pid = wait(&status);
        if (finished_pid > 0) {
            std::cout << "Child process with PID " << finished_pid << " has finished." << std::endl;
        }
    }

    std::cout << "All child processes have finished. Parent process is exiting." << std::endl;

    return 0;
}