#pragma once

#include "Events/EventHandler.h"
#include "Packet/Packet.h"
#include "Cache/RouteCache.h"
#include "Serialize/Serialize.h"
#include <iostream>
#include <iomanip>
#include <memory>
#include <atomic>
#include <thread>
#include <vector>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <string>


// Networking headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>


class BaseNode {
public:
    /**
     * @brief Constructs a BaseNode, initializes networking.
     * @param id The unique ID for this node.
     * @param rx_p The UDP port this node will listen on (Receive Only).
     * @param tx_p The UDP port this node will send from (Send Only).
     */
    BaseNode(uint8_t id, int rx_p, int tx_p, int loss_p = 0) 
        : node_id(id), rx_port(rx_p), tx_port(tx_p), loss_percentage(loss_p), stop_threads(false), rx_socket_fd(-1), tx_socket_fd(-1) {
        
        // 1. Create RX UDP socket
        rx_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (rx_socket_fd < 0) {
            throw std::runtime_error("Failed to create RX socket.");
        }

        // 2. Bind RX socket to the given RX port
        sockaddr_in rx_address{};
        rx_address.sin_family = AF_INET;
        rx_address.sin_addr.s_addr = INADDR_ANY;
        rx_address.sin_port = htons(rx_port);

        if (bind(rx_socket_fd, (const struct sockaddr *)&rx_address, sizeof(rx_address)) < 0) {
            throw std::runtime_error("Failed to bind RX socket to port " + std::to_string(rx_port));
        }

        // 3. Create TX UDP socket
        tx_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (tx_socket_fd < 0) {
            throw std::runtime_error("Failed to create TX socket.");
        }
        
        // Bind TX socket to TX port
        sockaddr_in tx_address{};
        tx_address.sin_family = AF_INET;
        tx_address.sin_addr.s_addr = INADDR_ANY;
        // Use INADDR_ANY for binding; in a real scenario we might bind to a specific interface.
        // Important: We must bind to tx_port so that receivers see traffic coming FROM tx_port? 
        // OR does it matter? The requirement says "Sending port will send data only".
        // Binding ensures the source port is constant.
        tx_address.sin_port = htons(tx_port);
        
        if (bind(tx_socket_fd, (const struct sockaddr *)&tx_address, sizeof(tx_address)) < 0) {
             throw std::runtime_error("Failed to bind TX socket to port " + std::to_string(tx_port));
        }

        std::cout << "Node " << (int)node_id << " initialized. RX: " << rx_port << ", TX: " << tx_port << std::endl;

        std::cout << "Node " << (int)node_id << " listening on RX: " << rx_port << ", Sending on TX: " << tx_port << std::endl;

        // 3. Start the receiver thread
        receiver_thread = std::thread(&BaseNode::receive_loop, this);

        // 4. Neighbors will be loaded via set_node_dir
        // load_neighbors();
    }

    void set_node_dir(const std::string& dir) {
        node_dir = dir;
        load_neighbors();
    }

    /**
     * @brief Destructor, cleans up network resources.
     */
    ~BaseNode() {
        stop_threads = true;
        if (receiver_thread.joinable()) {
            // Unblock the recvfrom call by sending a dummy packet to self
            // This is a common pattern to gracefully shut down a listening thread
            sockaddr_in self_addr{};
            self_addr.sin_family = AF_INET;
            self_addr.sin_port = htons(rx_port);
            self_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            char dummy = 'q';
            sendto(tx_socket_fd, &dummy, sizeof(dummy), 0, (struct sockaddr*)&self_addr, sizeof(self_addr));
            
            receiver_thread.join();
        }
        if (rx_socket_fd != -1) {
            close(rx_socket_fd);
        }
        if (tx_socket_fd != -1) {
            close(tx_socket_fd);
        }
    }

    void schedule_packet_forwarding(const std::shared_ptr<Packet>& packet) {
        auto task = [this, packet]() {
            auto forward_packet = std::make_shared<Packet>(*packet);
            forward_packet->hopcount++;

            if (forward_packet->hopcount > MAX_HOP_COUNT) {
                std::cout << "Packet dropped: TTL for packet to " << (int)forward_packet->destination_id << std::endl;
                packets_dropped++;
                return;
            }

            uint8_t next_hop_id;
            if (route_cache.get_next_hop(forward_packet->destination_id, next_hop_id)) {
                auto bytes_to_send = serialize_packet(*forward_packet);
                send_packet(next_hop_id, bytes_to_send);
                packets_forwarded++;
            } else {
                std::cout << "Packet dropped: No route to " << (int)forward_packet->destination_id << std::endl;
                packets_dropped++;
            }
        };
        forwarding_queue.push(Event(EventType::PACKET_OUTGOING, task));
    }

    virtual void schedule_packet_reception(const std::shared_ptr<Packet>& packet) {
        packets_received++;
        auto task = [this, packet]() {
            std::cout << "Processing packet from " << (int)packet->source_id << " with seq " << packet->sequence_number << std::endl;
            log_packet_event("RECEIVE", "Packet from " + std::to_string(packet->source_id) + " Seq: " + std::to_string(packet->sequence_number));
        };
        receiving_queue.push(Event(EventType::PACKET_INCOMING, task));
    }

    void schedule_route_update(uint8_t destination, uint8_t next_hop) {
        auto task = [this, destination, next_hop]() {
            route_cache.add_or_update(destination, next_hop);
            std::cout << "Route updated: " << (int)destination << " -> " << (int)next_hop << std::endl;
        };
        route_update_queue.push(Event(EventType::ROUTE_CACHE_UPDATE, task));
    }

    void process_events() {
        process_queue(forwarding_queue);
        process_queue(receiving_queue);
        process_queue(route_update_queue);
    }

    uint8_t get_node_id() const {
        return node_id;
    }

    void send_packet(uint8_t next_hop_id, const std::vector<uint8_t>& packet_data) {
        if (tx_socket_fd < 0) return;

        // Calculate RX port of the destination
        // RX_BASE = 8000. dest_rx_port = 8000 + next_hop_id.
        // We know our RX port is 8000 + node_id.
        // So dest_rx_port = rx_port - node_id + next_hop_id.
        int dest_rx_port = (rx_port - node_id) + next_hop_id;

        sockaddr_in dest_address{};
        dest_address.sin_family = AF_INET;
        dest_address.sin_port = htons(dest_rx_port);
        dest_address.sin_addr.s_addr = inet_addr("127.0.0.1");

        // Packet Loss Simulation
        if (loss_percentage > 0 && (std::rand() % 100) < loss_percentage) {
            std::cout << "Node " << (int)node_id << " DROPPED packet to " << (int)next_hop_id << " due to loss simulation." << std::endl;
            log_packet_event("DROP", "Loss simulation to " + std::to_string(next_hop_id));
            packet_loss_dropped++;
            return;
        }

        sendto(tx_socket_fd, packet_data.data(), packet_data.size(), 0, (const struct sockaddr *)&dest_address, sizeof(dest_address));
        
        log_packet_event("SEND", "To " + std::to_string(next_hop_id) + " (Bytes: " + std::to_string(packet_data.size()) + ")");
        
        // Count as sent only if it's the source (approximation, or add explicit 'originated' flag)
        // For now, let's assume if we are calling send_packet, we might be forwarding OR sending.
        // We'll increment packets_sent ONLY if we are the source. This requires deserializing or passing context.
        // Simplification: In DSRNode/BaseNode, when we create a NEW packet, we should increment.
    }

    void increment_packets_sent() {
        packets_sent++;
    }

    void log_packet_event(const std::string& action, const std::string& info) {
        if (node_dir.empty()) return;
        std::ofstream logfile(node_dir + "/PacketLog.txt", std::ios::app);
        if (logfile.is_open()) {
            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            logfile << std::put_time(std::localtime(&now), "%F %T") << " [" << action << "] " << info << "\n";
            logfile.close();
        }
    }

    void save_stats() {
        if (node_dir.empty()) {
             // Fallback or error
             return;
        }
        std::string stats_filepath = node_dir + "/Stats.txt";
        std::ofstream outfile(stats_filepath);
        if (outfile.is_open()) {
            outfile << "--- Node Statistics ---\n";
            outfile << "Packets Sent (Originated): " << packets_sent << "\n";
            outfile << "Packets Received: " << packets_received << "\n";
            outfile << "Packets Forwarded: " << packets_forwarded << "\n";
            outfile << "Packets Dropped: " << (packets_dropped + packet_loss_dropped) << "\n";
            
            if (packets_sent > 0) {
                 double pdr = (double)packets_received / (double)packets_sent; // This is naive PDR (received/sent globally). 
                 // For a single node, PDR usually means (Packets I received that were for me) / (Packets I sent).
                 // BaseNode::packets_received counts everything delivered to app layer? 
                 // schedule_packet_reception is called when dest == me. So yes.
                 outfile << "Packet Delivery Ratio: " << (pdr * 100.0) << "%\n";
            }

            outfile.close();
        }

        std::string cache_filepath = node_dir + "/RouteCache.txt";
        route_cache.save_to_file(cache_filepath);
    }

protected:
    uint8_t node_id;
    int rx_port;
    int tx_port;
    int loss_percentage;
    std::string node_dir;
    int rx_socket_fd;
    int tx_socket_fd;
    std::thread receiver_thread;
    std::atomic<bool> stop_threads;

    EventHandler forwarding_queue;
    EventHandler receiving_queue;
    EventHandler route_update_queue;
    RouteCache route_cache;

    std::vector<int> neighbors;

    std::atomic<uint64_t> packets_received{0};
    std::atomic<uint64_t> packets_forwarded{0};
    std::atomic<uint64_t> packets_dropped{0};
    std::atomic<uint64_t> packets_sent{0};
    std::atomic<uint64_t> packet_loss_dropped{0};

    static const uint8_t MAX_HOP_COUNT = 50;

    void broadcast(const std::vector<uint8_t>& packet_data) {
        log_packet_event("BROADCAST", "To all neighbors");
        for (int neighbor_id : neighbors) {
            // neighbors vector now contains Node IDs, not ports!
            send_packet(neighbor_id, packet_data);
        }
    }

    virtual void process_received_packet(std::shared_ptr<Packet> packet) {
        if (packet->destination_id == this->node_id) {
            schedule_packet_reception(packet);
        } else {
            schedule_packet_forwarding(packet);
        }
    }

private:
    void receive_loop() {
        char buffer[2048];
        sockaddr_in sender_address{};
        socklen_t sender_addr_len = sizeof(sender_address);

        while (!stop_threads) {
            ssize_t bytes_received = recvfrom(rx_socket_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&sender_address, &sender_addr_len);
            
            if (bytes_received < 0 || stop_threads) {
                break;
            }

            try {
                std::vector<uint8_t> received_data(buffer, buffer + bytes_received);
                auto packet_ptr = std::make_shared<Packet>(deserialize_packet(received_data));
                process_received_packet(packet_ptr);

            } catch (const std::exception& e) {
                std::cerr << "Error deserializing packet: " << e.what() << std::endl;
            }
        }
    }

    void load_neighbors() {
        if (node_dir.empty()) {
            std::cerr << "Error: Node directory not set for Node " << (int)node_id << std::endl;
            return;
        }
        std::filesystem::path filepath = node_dir;
        filepath /= "AccessTable.txt";
        std::ifstream infile(filepath);
        if (!infile.is_open()) {
            std::cerr << "Warning: Could not open access table file: " << filepath << std::endl;
            return;
        }

        int p;
        if (infile >> p) {
             // own port, do nothing
        }

        while (infile >> p) {
            neighbors.push_back(p);
        }
        infile.close();
        std::cout << "Node " << (int)node_id << " loaded " << neighbors.size() << " neighbors." << std::endl;
    }

    void process_queue(EventHandler& queue) {
        Event e;
        while (queue.try_pop(e)) {
            if (e.callback) {
                e.callback();
            }
        }
    }
};