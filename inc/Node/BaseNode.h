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

// PKCertChain Integration
#include "Blockchain/PKCertChain/Crypto.h"
#include "Blockchain/PKCertChain/Signkeys.h"
#include "Consensus/PoW.h"
#include "Helper/CSPRNG.h"

// RouteLogChain Integration
#include "Blockchain/RouteLogChain/Receipt.h"
#include "Blockchain/RouteLogChain/Blockchain.h"

class BaseNode {
public:
    BaseNode(uint8_t id, int rx_p, int tx_p, int loss_p = 0)
        : node_id(id), rx_port(rx_p), tx_port(tx_p), loss_percentage(loss_p),
          stop_threads(false), rx_socket_fd(-1), tx_socket_fd(-1) {

        // RX socket
        rx_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (rx_socket_fd < 0) throw std::runtime_error("Failed to create RX socket.");

        sockaddr_in rx_address{};
        rx_address.sin_family = AF_INET;
        rx_address.sin_addr.s_addr = INADDR_ANY;
        rx_address.sin_port = htons(rx_port);

        if (bind(rx_socket_fd, (const struct sockaddr*)&rx_address, sizeof(rx_address)) < 0)
            throw std::runtime_error("Failed to bind RX socket to port " + std::to_string(rx_port));

        // TX socket
        tx_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (tx_socket_fd < 0) throw std::runtime_error("Failed to create TX socket.");

        sockaddr_in tx_address{};
        tx_address.sin_family = AF_INET;
        tx_address.sin_addr.s_addr = INADDR_ANY;
        tx_address.sin_port = htons(tx_port);

        if (bind(tx_socket_fd, (const struct sockaddr*)&tx_address, sizeof(tx_address)) < 0)
            throw std::runtime_error("Failed to bind TX socket to port " + std::to_string(tx_port));

        std::cout << "Node " << (int)node_id << " initialized. RX: " << rx_port
                  << ", TX: " << tx_port << std::endl;

        receiver_thread = std::thread(&BaseNode::receive_loop, this);
    }

    /** PKCertChain + PoW identity setup **/
    void SetupIdentity(uint32_t assigned_id) {
        if (assigned_id != node_id) {
            std::cout << "Node " << (int)node_id << " re-assigned ID to " << assigned_id << std::endl;
            node_id = (uint8_t)assigned_id;
        }

        std::cout << "Node " << (int)node_id << " setting up PKCertChain identity..." << std::endl;

        if (!GenerateAndLogSignKeys(assigned_id))
            std::cerr << "Failed to generate signing keys\n";

        if (!GenerateAndLogX25519Key(assigned_id))
            std::cerr << "Failed to generate encryption keys\n";

        std::array<uint8_t, 32> dummyPub = {0};
        std::array<uint8_t, 64> dummySig = {0};
        PowChallenge* challenge = GeneratePoWChallenge(nullptr, (uint16_t)assigned_id, dummyPub, dummySig);

        std::cout << "Node " << (int)node_id << " solving PoW...\n";
        uint64_t nonce = SolvePowChallenge(*challenge);

        std::cout << "Solved PoW. Nonce: " << nonce << std::endl;
        delete challenge;
    }

    void set_node_dir(const std::string& dir) {
        node_dir = dir;
        load_neighbors();
    }

    ~BaseNode() {
        stop_threads = true;

        if (receiver_thread.joinable()) {
            sockaddr_in self_addr{};
            self_addr.sin_family = AF_INET;
            self_addr.sin_port = htons(rx_port);
            self_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            char dummy = 'q';
            sendto(tx_socket_fd, &dummy, sizeof(dummy), 0,
                   (struct sockaddr*)&self_addr, sizeof(self_addr));
            receiver_thread.join();
        }

        if (rx_socket_fd != -1) close(rx_socket_fd);
        if (tx_socket_fd != -1) close(tx_socket_fd);
    }

    /** Packet forwarding with RouteLogChain receipt + blockchain update **/
    void schedule_packet_forwarding(const std::shared_ptr<Packet>& packet) {
        auto task = [this, packet]() {
            auto forward_packet = std::make_shared<Packet>(*packet);
            forward_packet->hopcount++;

            if (forward_packet->hopcount > MAX_HOP_COUNT) {
                std::cout << "Packet dropped: TTL expired\n";
                packets_dropped++;
                return;
            }

            uint8_t next_hop_id;
            if (route_cache.get_next_hop(forward_packet->destination_id, next_hop_id)) {

                // RouteLogChain Receipt
                std::string route_id =
                    std::to_string(forward_packet->source_id) + "-" +
                    std::to_string(forward_packet->destination_id);

                Receipt receipt(node_id, route_id, "DATA_forwarded", forward_packet->sequence_number);
                local_receipts.push_back(receipt);
                local_blockchain.process_receipt(receipt);
                log_packet_event("RECEIPT", receipt.ToString());

                auto bytes = serialize_packet(*forward_packet);
                send_packet(next_hop_id, bytes);
                packets_forwarded++;

            } else {
                std::cout << "Packet dropped: No route\n";
                packets_dropped++;
            }
        };
        forwarding_queue.push(Event(EventType::PACKET_OUTGOING, task));
    }

    virtual void schedule_packet_reception(const std::shared_ptr<Packet>& packet) {
        packets_received++;
        auto task = [this, packet]() {
            log_packet_event("RECEIVE",
                "Packet from " + std::to_string(packet->source_id) +
                " Seq: " + std::to_string(packet->sequence_number));
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

    uint8_t get_node_id() const { return node_id; }

    void send_packet(uint8_t next_hop_id, const std::vector<uint8_t>& data) {
        if (tx_socket_fd < 0) return;

        int dest_rx_port = (rx_port - node_id) + next_hop_id;

        sockaddr_in dest{};
        dest.sin_family = AF_INET;
        dest.sin_port = htons(dest_rx_port);
        dest.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (loss_percentage > 0 && (std::rand() % 100) < loss_percentage) {
            log_packet_event("DROP", "Loss simulation");
            packet_loss_dropped++;
            return;
        }

        sendto(tx_socket_fd, data.data(), data.size(), 0,
               (struct sockaddr*)&dest, sizeof(dest));
        log_packet_event("SEND", "To " + std::to_string(next_hop_id));
    }

    void increment_packets_sent() { packets_sent++; }

    void log_packet_event(const std::string& action, const std::string& info) {
        if (node_dir.empty()) return;

        std::ofstream logfile(node_dir + "/PacketLog.txt", std::ios::app);
        if (!logfile.is_open()) return;

        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        logfile << std::put_time(std::localtime(&now), "%F %T")
                << " [" << action << "] " << info << "\n";
    }

    void save_stats() {
        if (node_dir.empty()) return;

        std::ofstream outfile(node_dir + "/Stats.txt");
        if (outfile.is_open()) {
            outfile << "--- Node Statistics ---\n";
            outfile << "Packets Sent: " << packets_sent << "\n";
            outfile << "Packets Received: " << packets_received << "\n";
            outfile << "Packets Forwarded: " << packets_forwarded << "\n";
            outfile << "Packets Dropped: " << (packets_dropped + packet_loss_dropped) << "\n";

            if (packets_sent > 0) {
                double pdr = (double)packets_received / (double)packets_sent;
                outfile << "Packet Delivery Ratio: " << (pdr * 100.0) << "%\n";
            }
        }

        route_cache.save_to_file(node_dir + "/RouteCache.txt");
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

    // RouteLogChain
    Blockchain local_blockchain;
    std::vector<Receipt> local_receipts;

    std::vector<int> neighbors;

    std::atomic<uint64_t> packets_received{0};
    std::atomic<uint64_t> packets_forwarded{0};
    std::atomic<uint64_t> packets_dropped{0};
    std::atomic<uint64_t> packets_sent{0};
    std::atomic<uint64_t> packet_loss_dropped{0};

    static const uint8_t MAX_HOP_COUNT = 50;

    void broadcast(const std::vector<uint8_t>& data) {
        log_packet_event("BROADCAST", "To neighbors");
        for (int n : neighbors) send_packet(n, data);
    }

    virtual void process_received_packet(std::shared_ptr<Packet> packet) {
        if (packet->destination_id == node_id)
            schedule_packet_reception(packet);
        else
            schedule_packet_forwarding(packet);
    }

private:
    void receive_loop() {
        char buffer[2048];
        sockaddr_in sender{};
        socklen_t len = sizeof(sender);

        while (!stop_threads) {
            ssize_t received = recvfrom(rx_socket_fd, buffer, sizeof(buffer), 0,
                                        (struct sockaddr*)&sender, &len);
            if (received < 0 || stop_threads) break;

            try {
                std::vector<uint8_t> data(buffer, buffer + received);
                auto packet = std::make_shared<Packet>(deserialize_packet(data));
                process_received_packet(packet);
            } catch (std::exception& e) {
                std::cerr << "Deserialize error: " << e.what() << std::endl;
            }
        }
    }

    void load_neighbors() {
        if (node_dir.empty()) return;

        std::ifstream infile(node_dir + "/AccessTable.txt");
        if (!infile.is_open()) return;

        int n;
        while (infile >> n) neighbors.push_back(n);

        std::cout << "Node " << (int)node_id << " loaded "
                  << neighbors.size() << " neighbors.\n";
    }

    void process_queue(EventHandler& q) {
        Event e;
        while (q.try_pop(e)) if (e.callback) e.callback();
    }
};
