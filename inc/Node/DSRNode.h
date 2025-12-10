#pragma once

#include "Node/BaseNode.h"
#include "Packet/Packet.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <map>
#include <chrono>
#include <string>
#include "../Blockchain/RouteLogChain/Receipt.h"

struct PendingAck {
    uint32_t sequence_number;
    uint8_t next_hop_id;
    std::chrono::steady_clock::time_point send_time;
    int retries;
    std::shared_ptr<Packet> original_packet;
};

class DSRNode : public BaseNode {
public:
    DSRNode(uint8_t id, int rx_p, int tx_p, int loss_p = 0);

    // --- DSR-Specific Methods ---
    void start_route_discovery(uint8_t destination);
    void handle_rreq(std::shared_ptr<Packet> packet);
    void handle_rrep(std::shared_ptr<Packet> packet);
    void handle_route_error(std::shared_ptr<Packet> packet);
    void process_received_packet(std::shared_ptr<Packet> packet) override;

    void save_dsr_routes();
    void process_events(); // Override to check for timeouts

    void send_data(uint8_t destination_id, const std::string& message);

    // RouteLogChain
    std::vector<Receipt> extract_receipts();

private:
    std::unordered_map<uint8_t, std::vector<uint8_t>> dsr_routes;
    
    // ACK and Route Maintenance
    std::map<uint32_t, PendingAck> pending_acks; // Key: Packet Sequence Number
    static const int MAX_ACK_RETRIES = 3;
    static const int ACK_TIMEOUT_MS = 1000; // 1 second timeout for simulation

    void send_rrep_back(std::vector<uint8_t> path);
    void send_ack(uint8_t next_hop, uint32_t seq_num);
    void check_ack_timeouts();
    void handle_ack(std::shared_ptr<Packet> packet);
    void route_failed(uint8_t next_hop_id);
};
