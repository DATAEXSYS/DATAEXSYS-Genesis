#include "Node/DSRNode.h"
#include "Packet/Packet.h"
#include "Serialize/Serialize.h"
#include <iostream>
#include <memory>
#include <algorithm>
#include <vector>

DSRNode::DSRNode(uint8_t id, int rx_p, int tx_p, int loss_p)
    : BaseNode(id, rx_p, tx_p, loss_p) {
    std::cout << "DSRNode " << (int)id << " created." << std::endl;
}

void DSRNode::process_received_packet(std::shared_ptr<Packet> packet){
    switch (packet->type) {
        case PacketType::RREQ:
            handle_rreq(packet);
            break;

        case PacketType::RREP:
            handle_rrep(packet);
            break;

        case PacketType::RERR:
            handle_route_error(packet);
            break;

        case PacketType::ACK:
            handle_ack(packet);
            break;

        case PacketType::DATA: {
            // --- ACK previous hop ---
            if (packet->destination_id == get_node_id() || packet->hopAddresses.size() > 0) {
                auto it = std::find(packet->hopAddresses.begin(), packet->hopAddresses.end(), get_node_id());

                if (it != packet->hopAddresses.begin() && it != packet->hopAddresses.end()) {
                    uint8_t prev_hop = *std::prev(it);
                    send_ack(prev_hop, packet->sequence_number);

                } else if (it == packet->hopAddresses.end() &&
                           packet->hopAddresses.size() > 0 &&
                           packet->destination_id == get_node_id()) {

                    uint8_t prev_hop = packet->hopAddresses.back();
                    send_ack(prev_hop, packet->sequence_number);
                }
            }

            // --- Destination Handling ---
            if (packet->destination_id == get_node_id()) {
                auto task = [this, packet]() {
                    std::cout << "DSR DATA packet received at final destination "
                              << (int)get_node_id() << std::endl;

                    log_packet_event("RECEIVE", "DSR DATA from " +
                                     std::to_string(packet->source_id));

                    // RouteLogChain
                    std::string rid = std::to_string(packet->source_id)
                                      + "->" + std::to_string(packet->destination_id);

                    Receipt rx(get_node_id(), rid, "PACKET_RECEIVED",
                               packet->sequence_number, "", "");

                    local_receipts.push_back(rx);
                };

                receiving_queue.push(Event(EventType::PACKET_INCOMING, task));
                packets_received++;

            } else {
                // --- Forward DATA ---
                auto task = [this, packet]() {
                    auto it = std::find(packet->hopAddresses.begin(),
                                        packet->hopAddresses.end(),
                                        get_node_id());

                    if (it != packet->hopAddresses.end() && std::next(it) != packet->hopAddresses.end()) {
                        uint8_t next_hop_id = *std::next(it);

                        std::cout << "Forwarding DSR DATA packet to next hop "
                                  << (int)next_hop_id << std::endl;

                        log_packet_event("FORWARD", "DSR DATA to " +
                                         std::to_string(next_hop_id));

                        send_packet(next_hop_id, serialize_packet(*packet));

                        // RouteLogChain
                        std::string rid = std::to_string(packet->source_id) +
                                          "->" + std::to_string(packet->destination_id);

                        Receipt rx(get_node_id(), rid, "DATA_forwarded",
                                   packet->sequence_number, "",
                                   std::to_string(next_hop_id));

                        local_receipts.push_back(rx);

                        // Track for ACK
                        PendingAck ack_info;
                        ack_info.sequence_number = packet->sequence_number;
                        ack_info.next_hop_id = next_hop_id;
                        ack_info.send_time = std::chrono::steady_clock::now();
                        ack_info.retries = 0;
                        ack_info.original_packet = packet;

                        pending_acks[packet->sequence_number] = ack_info;

                    } else {
                        std::cerr << "Could not find next hop at node "
                                  << (int)get_node_id() << std::endl;
                        log_packet_event("ERROR",
                                         "No next hop for DATA to " +
                                         std::to_string(packet->destination_id));
                    }
                };

                forwarding_queue.push(Event(EventType::PACKET_OUTGOING, task));
            }
            break;
        }

        default:
            BaseNode::process_received_packet(packet);
            break;
    }
}

void DSRNode::send_data(uint8_t destination_id, const std::string& message) {
    if (!has_route(destination_id)) {
        std::cout << "No route to " << (int)destination_id
                  << ". Starting discovery." << std::endl;
        start_route_discovery(destination_id);
        return;
    }

    auto route = dsr_routes[destination_id];
    if (route.empty()) return;

    Packet packet;
    packet.type = PacketType::DATA;
    packet.source_id = get_node_id();
    packet.destination_id = destination_id;
    packet.hopAddresses = route;
    packet.timestamp = std::time(nullptr);
    packet.payload.assign(message.begin(), message.end());
    packet.sequence_number = (uint32_t)std::rand();
    packet.hopcount = route.size();

    if (route.size() > 1) {
        uint8_t next_hop = route[1];

        std::cout << "Sending DATA to " << (int)destination_id
                  << " via " << (int)next_hop << std::endl;

        send_packet(next_hop, serialize_packet(packet));
        increment_packets_sent();

        PendingAck ack_info;
        ack_info.sequence_number = packet.sequence_number;
        ack_info.next_hop_id = next_hop;
        ack_info.send_time = std::chrono::steady_clock::now();
        ack_info.retries = 0;
        ack_info.original_packet = std::make_shared<Packet>(packet);

        pending_acks[packet.sequence_number] = ack_info;

        log_packet_event("SEND_DATA",
                         "Dest: " + std::to_string(destination_id) +
                         " Seq: " + std::to_string(packet.sequence_number));
    }
}

bool DSRNode::has_route(uint8_t destination_id) const {
    return dsr_routes.count(destination_id) &&
           !dsr_routes.at(destination_id).empty();
}

void DSRNode::start_route_discovery(uint8_t destination_id) {
    std::cout << "Node " << (int)get_node_id()
              << " starting route discovery for "
              << (int)destination_id << std::endl;

    Packet rreq;
    rreq.type = PacketType::RREQ;
    rreq.source_id = get_node_id();
    rreq.destination_id = destination_id;
    rreq.hopAddresses.push_back(get_node_id());
    rreq.hopcount = rreq.hopAddresses.size();

    broadcast(serialize_packet(rreq));
}

void DSRNode::handle_rreq(std::shared_ptr<Packet> packet) {
    // Loop detection
    if (std::find(packet->hopAddresses.begin(),
                  packet->hopAddresses.end(),
                  get_node_id()) != packet->hopAddresses.end())
    {
        return;
    }

    packet->hopAddresses.push_back(get_node_id());
    packet->hopcount = packet->hopAddresses.size();

    std::cout << "Node " << (int)get_node_id()
              << " handling RREQ. Path: ";
    for (auto id : packet->hopAddresses) std::cout << (int)id << " ";
    std::cout << std::endl;

    if (packet->destination_id == get_node_id()) {
        std::cout << "RREQ reached destination "
                  << (int)get_node_id() << ". Sending RREP back." << std::endl;

        dsr_routes[packet->source_id] = packet->hopAddresses;
        send_rrep_back(packet->hopAddresses);

    } else {
        broadcast(serialize_packet(*packet));
    }
}

void DSRNode::handle_rrep(std::shared_ptr<Packet> packet) {
    std::cout << "Node " << (int)get_node_id()
              << " received RREP." << std::endl;

    std::vector<uint8_t> forward_path = packet->hopAddresses;
    std::reverse(forward_path.begin(), forward_path.end());
    dsr_routes[packet->source_id] = forward_path;

    if (packet->destination_id != get_node_id()) {
        auto it = std::find(packet->hopAddresses.begin(),
                            packet->hopAddresses.end(),
                            get_node_id());

        if (it != packet->hopAddresses.end() && std::next(it) != packet->hopAddresses.end()) {
            uint8_t next_hop = *std::next(it);
            send_packet(next_hop, serialize_packet(*packet));
        } else {
            std::cerr << "RREP forwarding failed at node "
                      << (int)get_node_id() << std::endl;
        }
    } else {
        std::cout << "RREP reached original requester "
                  << (int)get_node_id() << ". Route OK." << std::endl;
    }
}

void DSRNode::handle_route_error(std::shared_ptr<Packet> packet) {
    std::cout << "Node " << (int)get_node_id()
              << " received Route Error." << std::endl;
    log_packet_event("RERR", "Received RERR");
}

void DSRNode::send_rrep_back(std::vector<uint8_t> path) {
    std::reverse(path.begin(), path.end());

    Packet rrep;
    rrep.type = PacketType::RREP;
    rrep.source_id = get_node_id();
    rrep.destination_id = path.back();
    rrep.hopAddresses = path;
    rrep.hopcount = path.size();

    if (path.size() > 1)
        send_packet(path[1], serialize_packet(rrep));
}

void DSRNode::process_events() {
    BaseNode::process_events();
    check_ack_timeouts();
}

void DSRNode::send_ack(uint8_t next_hop, uint32_t seq_num) {
    Packet ack;
    ack.type = PacketType::ACK;
    ack.source_id = get_node_id();
    ack.destination_id = next_hop;
    ack.sequence_number = seq_num;
    ack.timestamp = std::time(nullptr);

    send_packet(next_hop, serialize_packet(ack));
}

void DSRNode::check_ack_timeouts() {
    auto now = std::chrono::steady_clock::now();
    std::vector<uint32_t> erase_list;

    for (auto& [seq, info] : pending_acks) {
        long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now - info.send_time).count();

        if (elapsed > ACK_TIMEOUT_MS) {
            info.retries++;

            if (info.retries > MAX_ACK_RETRIES) {
                std::cout << "Node " << (int)get_node_id()
                          << ": ACK timeout seq " << seq
                          << " link broken to " << (int)info.next_hop_id
                          << std::endl;

                erase_list.push_back(seq);
                route_failed(info.next_hop_id);

            } else {
                std::cout << "Node " << (int)get_node_id()
                          << ": Retrying seq " << seq
                          << " to " << (int)info.next_hop_id
                          << " retry " << info.retries << std::endl;

                send_packet(info.next_hop_id,
                            serialize_packet(*info.original_packet));
                info.send_time = now;
            }
        }
    }

    for (uint32_t seq : erase_list)
        pending_acks.erase(seq);
}

void DSRNode::handle_ack(std::shared_ptr<Packet> packet) {
    if (pending_acks.count(packet->sequence_number)) {
        log_packet_event("ACK",
                         "Received Seq " + std::to_string(packet->sequence_number));
        pending_acks.erase(packet->sequence_number);
    }
}

void DSRNode::route_failed(uint8_t next_hop_id) {
    route_cache.remove_routes_with_next_hop(next_hop_id);

    std::cout << "Node " << (int)get_node_id()
              << " removed broken routes via "
              << (int)next_hop_id << std::endl;

    Packet rerr;
    rerr.type = PacketType::RERR;
    rerr.source_id = get_node_id();
    rerr.destination_id = 255;
    rerr.payload.push_back(next_hop_id);

    std::cout << "Node " << (int)get_node_id()
              << " generating RERR for broken link "
              << (int)next_hop_id << std::endl;
}

void DSRNode::save_dsr_routes() {
    if (node_dir.empty()) return;

    std::ofstream outfile(node_dir + "/DSR_RouteCache.txt");
    if (!outfile.is_open()) return;

    outfile << "--- DSR Route Cache ---\n";

    for (auto& [dst, path] : dsr_routes) {
        outfile << "Destination: " << (int)dst << " Path: ";
        for (size_t i = 0; i < path.size(); ++i)
            outfile << (int)path[i] << (i + 1 < path.size() ? " -> " : "");
        outfile << "\n";
    }
}

std::vector<Receipt> DSRNode::extract_receipts() {
    auto tmp = local_receipts;
    local_receipts.clear();
    return tmp;
}
