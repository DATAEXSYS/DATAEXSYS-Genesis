#pragma once

#include "Events/EventHandler.h"
#include "Packet/Packet.h"
#include "Cache/RouteCache.h"
#include "Serialize/Serialize.h"
#include <iostream>
#include <memory>
#include <atomic>
#include <thread>
#include <vector>
#include <stdexcept>

// Networking headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class BaseNode {
public:
    /**
     * @brief Constructs a BaseNode, initializes networking.
     * @param id The unique ID for this node.
     * @param port The UDP port this node will listen on.
     */
    BaseNode(uint8_t id, int p) : node_id(id), port(p), stop_threads(false), socket_fd(-1) {
        // 1. Create UDP socket
        socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd < 0) {
            throw std::runtime_error("Failed to create socket.");
        }

        // 2. Bind socket to the given port
        sockaddr_in self_address{};
        self_address.sin_family = AF_INET;
        self_address.sin_addr.s_addr = INADDR_ANY;
        self_address.sin_port = htons(port);

        if (bind(socket_fd, (const struct sockaddr *)&self_address, sizeof(self_address)) < 0) {
            throw std::runtime_error("Failed to bind socket to port " + std::to_string(port));
        }

        std::cout << "Node " << (int)node_id << " listening on port " << port << std::endl;

        // 3. Start the receiver thread
        receiver_thread = std::thread(&BaseNode::receive_loop, this);
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
            self_addr.sin_port = htons(port);
            self_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            char dummy = 'q';
            sendto(socket_fd, &dummy, sizeof(dummy), 0, (struct sockaddr*)&self_addr, sizeof(self_addr));
            
            receiver_thread.join();
        }
        if (socket_fd != -1) {
            close(socket_fd);
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

    void schedule_packet_reception(const std::shared_ptr<Packet>& packet) {
        auto task = [this, packet]() {
            std::cout << "Processing packet from " << (int)packet->source_id << " with seq " << packet->sequence_number << std::endl;
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

private:
    uint8_t node_id;
    int port;
    int socket_fd;
    std::thread receiver_thread;
    std::atomic<bool> stop_threads;

    EventHandler forwarding_queue;
    EventHandler receiving_queue;
    EventHandler route_update_queue;
    RouteCache route_cache;

    std::atomic<uint64_t> packets_forwarded{0};
    std::atomic<uint64_t> packets_dropped{0};

    static const uint8_t MAX_HOP_COUNT = 50;

    /**
     * @brief Main loop for the receiver thread. Listens for and handles incoming packets.
     */
    void receive_loop() {
        char buffer[2048]; // Buffer for incoming data
        sockaddr_in sender_address{};
        socklen_t sender_addr_len = sizeof(sender_address);

        while (!stop_threads) {
            ssize_t bytes_received = recvfrom(socket_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&sender_address, &sender_addr_len);
            
            if (bytes_received < 0 || stop_threads) {
                // Error or shutdown signal
                break;
            }

            try {
                std::vector<uint8_t> received_data(buffer, buffer + bytes_received);
                auto packet_ptr = std::make_shared<Packet>(deserialize_packet(received_data));

                if (packet_ptr->destination_id == this->node_id) {
                    schedule_packet_reception(packet_ptr);
                } else {
                    schedule_packet_forwarding(packet_ptr);
                }
            } catch (const std::exception& e) {
                std::cerr << "Error deserializing packet: " << e.what() << std::endl;
            }
        }
    }

    /**
     * @brief Sends a packet to a specified next-hop node via UDP.
     * @param next_hop_id The ID of the node to send the packet to.
     * @param packet_data The serialized byte vector of the packet.
     */
    void send_packet(uint8_t next_hop_id, const std::vector<uint8_t>& packet_data) {
        if (socket_fd < 0) return;

        sockaddr_in dest_address{};
        dest_address.sin_family = AF_INET;
        dest_address.sin_port = htons(8080 + next_hop_id); // Assumes port = 8080 + node_id
        dest_address.sin_addr.s_addr = inet_addr("127.0.0.1");

        sendto(socket_fd, packet_data.data(), packet_data.size(), 0, (const struct sockaddr *)&dest_address, sizeof(dest_address));
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