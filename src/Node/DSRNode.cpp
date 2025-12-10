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
            // ACK the previous hop if it's unicast
            if (packet->destination_id == get_node_id() || packet->hopAddresses.size() > 0) {
                 // Find pure previous hop for ACK
                 // For now, assume we can deduce previous hop from link layer or packet metadata
                 // But since we are at app layer, let's find our current position in hopAddresses
                 auto it = std::find(packet->hopAddresses.begin(), packet->hopAddresses.end(), get_node_id());
                 if (it != packet->hopAddresses.begin() && it != packet->hopAddresses.end()) {
                     uint8_t prev_hop = *std::prev(it);
                     send_ack(prev_hop, packet->sequence_number);
                 } else if (it == packet->hopAddresses.end() && packet->hopAddresses.size() > 0 && packet->destination_id == get_node_id()) {
                     // We are destination, prev hop is last in list
                     uint8_t prev_hop = packet->hopAddresses.back();
                     send_ack(prev_hop, packet->sequence_number);
                 }
            }
            // If this node is the destination, receive it.
            if (packet->destination_id == get_node_id()) {
                auto task = [this, packet]() {
                    std::cout << "DSR DATA packet received at final destination " << (int)get_node_id() << std::endl;
                    log_packet_event("RECEIVE", "DSR DATA from " + std::to_string(packet->source_id));
                };
                receiving_queue.push(Event(EventType::PACKET_INCOMING, task));
                packets_received++; 
            } else {
                // Otherwise, forward it along the source route.
                auto task = [this, packet]() {
                    // Find this node in the hop addresses
                    auto it = std::find(packet->hopAddresses.begin(), packet->hopAddresses.end(), get_node_id());
                    if (it != packet->hopAddresses.end() && std::next(it) != packet->hopAddresses.end()) {
                        uint8_t next_hop_id = *std::next(it);
                        std::cout << "Forwarding DSR DATA packet to next hop " << (int)next_hop_id << std::endl;
                        log_packet_event("FORWARD", "DSR DATA to " + std::to_string(next_hop_id));
                        send_packet(next_hop_id, serialize_packet(*packet));
                        
                        // Track for ACK
                        PendingAck ack_info;
                        ack_info.sequence_number = packet->sequence_number;
                        ack_info.next_hop_id = next_hop_id;
                        ack_info.send_time = std::chrono::steady_clock::now();
                        ack_info.retries = 0;
                        ack_info.original_packet = packet;
                        pending_acks[packet->sequence_number] = ack_info;

                    } else {
                        std::cerr << "Could not find next hop for DSR DATA packet at node " << (int)get_node_id() << std::endl;
                        log_packet_event("ERROR", "No next hop for DSR DATA to " + std::to_string(packet->destination_id));
                    }
                };
                forwarding_queue.push(Event(EventType::PACKET_OUTGOING, task));
            }
            break;
        }
        default:
            // For any other packet type, use the base node's logic.
            BaseNode::process_received_packet(packet);
            break;
    }
}

void DSRNode::send_data(uint8_t destination_id, const std::string& message) {
    if (dsr_routes.find(destination_id) == dsr_routes.end()) {
        std::cout << "No route to " << (int)destination_id << ". Starting discovery." << std::endl;
        start_route_discovery(destination_id);
        return;
    }

    std::vector<uint8_t> route = dsr_routes[destination_id];
    if (route.empty()) return;

    Packet packet;
    packet.type = PacketType::DATA;
    packet.source_id = get_node_id();
    packet.destination_id = destination_id;
    packet.hopAddresses = route;
    packet.timestamp = std::time(nullptr);
    packet.payload.assign(message.begin(), message.end());
    // packet.sequence_number is initialized to 0 by default constructor
    // Use a static counter or random for more robustness if needed, 
    // but for now relying on BaseNode not to overwrite it if we set it? 
    // Actually Packet struct doesn't auto-increment. Let's start with rand or simple counter.
    packet.sequence_number = (uint32_t)std::rand();
    
    // First hop is the second element in the route (first is source)
    if (route.size() > 1) {
        uint8_t next_hop = route[1];
        std::cout << "Sending DATA to " << (int)destination_id << " via " << (int)next_hop << std::endl;
        send_packet(next_hop, serialize_packet(packet));
        increment_packets_sent();
        
        // Track for ACK
        PendingAck ack_info;
        ack_info.sequence_number = packet.sequence_number;
        ack_info.next_hop_id = next_hop;
        ack_info.send_time = std::chrono::steady_clock::now();
        ack_info.retries = 0;
        ack_info.original_packet = std::make_shared<Packet>(packet);
        pending_acks[packet.sequence_number] = ack_info;
        
        log_packet_event("SEND_DATA", "Dest: " + std::to_string(destination_id) + " Seq: " + std::to_string(packet.sequence_number));
    }
}

void DSRNode::start_route_discovery(uint8_t destination_id) {
    std::cout << "Node " << (int)get_node_id() << " starting route discovery for " << (int)destination_id << std::endl;
    Packet rreq;
    rreq.type = PacketType::RREQ;
    rreq.source_id = get_node_id();
    rreq.destination_id = destination_id;
    rreq.hopAddresses.push_back(get_node_id());

    auto bytes = serialize_packet(rreq);
    broadcast(bytes);
    increment_packets_sent();
}

void DSRNode::handle_rreq(std::shared_ptr<Packet> packet) {
    // 1. Append current node's ID to the hop address list in the packet.
    packet->hopAddresses.push_back(get_node_id());
    log_packet_event("HANDLE", "RREQ from " + std::to_string(packet->source_id));
    std::cout << "Node " << (int)get_node_id() << " handling RREQ. Path so far: ";
    for(uint8_t id : packet->hopAddresses) {
        std::cout << (int)id << " ";
    }
    std::cout << std::endl;

    // 2. Check if the current node is the destination.
    if (packet->destination_id == get_node_id()) {
        std::cout << "RREQ reached destination " << (int)get_node_id() << ". Sending RREP back." << std::endl;
        
        // Store the discovered route.
        dsr_routes[packet->source_id] = packet->hopAddresses;
        
        // Send a Route Reply (RREP) back to the source.
        send_rrep_back(packet->hopAddresses);
        increment_packets_sent(); // Count RREP as sent

    } else {
        // 3. If not the destination, broadcast the RREQ to neighbors.
        std::cout << "RREQ not for me. Broadcasting further." << std::endl;
        auto bytes = serialize_packet(*packet);
        broadcast(bytes);
    }
}

void DSRNode::handle_rrep(std::shared_ptr<Packet> packet) {
    std::cout << "Node " << (int)get_node_id() << " received an RREP." << std::endl;
    log_packet_event("HANDLE", "RREP from " + std::to_string(packet->source_id));

    // Store the route from the RREP.
    dsr_routes[packet->source_id] = packet->hopAddresses;

    // If this node is not the original source of the RREQ, forward the RREP.
    if (packet->destination_id != get_node_id()) {
         // Find this node in the hop addresses
        auto it = std::find(packet->hopAddresses.begin(), packet->hopAddresses.end(), get_node_id());
        if (it != packet->hopAddresses.end() && std::next(it) != packet->hopAddresses.end()) {
            uint8_t next_hop_id = *std::next(it);
            std::cout << "Forwarding RREP to next hop " << (int)next_hop_id << std::endl;
            send_packet(next_hop_id, serialize_packet(*packet));
        } else {
            std::cerr << "Could not find next hop for RREP at node " << (int)get_node_id() << std::endl;
        }
    } else {
        std::cout << "RREP reached original requester " << (int)get_node_id() << ". Route discovered!" << std::endl;
    }
}

void DSRNode::handle_route_error(std::shared_ptr<Packet> packet) {
    // Placeholder for route error handling.
    std::cout << "Node " << (int)get_node_id() << " received a Route Error." << std::endl;
    log_packet_event("RERR", "Received RERR");
    // In a full implementation, this would involve removing broken links/routes.
}

void DSRNode::send_rrep_back(std::vector<uint8_t> path) {
    // The path in the RREQ is from source to destination.
    // The RREP needs to travel the reverse path.
    std::reverse(path.begin(), path.end());

    Packet rrep;
    rrep.type = PacketType::RREP;
    rrep.source_id = get_node_id(); // The destination of the RREQ is the source of the RREP.
    rrep.destination_id = path.back();   // The final destination of the RREP is the source of the RREQ.
    rrep.hopAddresses = path;

    // The first hop for the RREP is the second node in the reversed path.
    if (path.size() > 1) {
        uint8_t next_hop_id = path[1];
        auto bytes = serialize_packet(rrep);
        send_packet(next_hop_id, bytes);
    }
}


void DSRNode::process_events() {
    BaseNode::process_events(); // Process queues
    check_ack_timeouts();       // Check for broken links
}

void DSRNode::send_ack(uint8_t next_hop, uint32_t seq_num) {
    Packet ack_packet;
    ack_packet.type = PacketType::ACK;
    ack_packet.source_id = get_node_id();
    ack_packet.destination_id = next_hop;
    ack_packet.sequence_number = seq_num; // ACK refers to this sequence number
    ack_packet.timestamp = std::time(nullptr);
    
    // Direct send (no routing needed for single hop ACK)
    send_packet(next_hop, serialize_packet(ack_packet));
    // std::cout << "Node " << (int)get_node_id() << " sent ACK for seq " << seq_num << " to " << (int)next_hop << std::endl;
}

void DSRNode::check_ack_timeouts() {
    auto now = std::chrono::steady_clock::now();
    std::vector<uint32_t> timed_out_acks;

    for (auto& pair : pending_acks) {
        auto& ack_info = pair.second;
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - ack_info.send_time).count();

        if (duration > ACK_TIMEOUT_MS) {
            ack_info.retries++;
            if (ack_info.retries > MAX_ACK_RETRIES) {
                std::cout << "Node " << (int)get_node_id() << ": ACK timeout for seq " << pair.first << " from " << (int)ack_info.next_hop_id << ". Link BROKEN." << std::endl;
                timed_out_acks.push_back(pair.first);
                route_failed(ack_info.next_hop_id);
            } else {
                // Retransmit
                std::cout << "Node " << (int)get_node_id() << ": Retrying packet " << pair.first << " to " << (int)ack_info.next_hop_id << " (Retry " << ack_info.retries << ")" << std::endl;
                send_packet(ack_info.next_hop_id, serialize_packet(*ack_info.original_packet));
                ack_info.send_time = now; // Reset timer
            }
        }
    }

    for (uint32_t seq : timed_out_acks) {
        pending_acks.erase(seq);
    }
}

void DSRNode::handle_ack(std::shared_ptr<Packet> packet) {
    // packet->sequence_number is the seq num being acknowledged
    if (pending_acks.count(packet->sequence_number)) {
        // std::cout << "Node " << (int)get_node_id() << " received ACK for seq " << packet->sequence_number << std::endl;
        log_packet_event("ACK", "Received for Seq " + std::to_string(packet->sequence_number));
        pending_acks.erase(packet->sequence_number);
    }
}

void DSRNode::route_failed(uint8_t next_hop_id) {
    // 1. Remove routes using this link
    route_cache.remove_routes_with_next_hop(next_hop_id);
    std::cout << "Node " << (int)get_node_id() << " removed routes using broken link to " << (int)next_hop_id << std::endl;

    // 2. Generate RERR (simplified: just broadcast to everyone for now, or send to source if known)
    // In standard DSR, we should reverse the source route of the failed packet and send RERR to source.
    // For this simulation, we'll create a RERR and broadcast it (or flood it) so the source eventually hears it.
    
    Packet rerr;
    rerr.type = PacketType::RERR;
    rerr.source_id = get_node_id();
    rerr.destination_id = 255; // Broadcast
    rerr.payload.push_back(next_hop_id); // The broken node
    
    // Broadcast RERR
    // broadcast(serialize_packet(rerr)); 
    // Optimization: Just print for now as full recursive RERR is complex
    std::cout << "Node " << (int)get_node_id() << " generating RERR: Link to " << (int)next_hop_id << " is broken." << std::endl;
}

void DSRNode::save_dsr_routes() {
    if (node_dir.empty()) return;
    std::string filepath = node_dir + "/DSR_RouteCache.txt";
    std::ofstream outfile(filepath);
    if (!outfile.is_open()) {
        return;
    }

    outfile << "--- DSR Route Cache ---\n";
    for (const auto& pair : dsr_routes) {
        outfile << "Destination: " << (int)pair.first << " Path: ";
        for (size_t i = 0; i < pair.second.size(); ++i) {
            outfile << (int)pair.second[i] << (i == pair.second.size() - 1 ? "" : " -> ");
        }
        outfile << "\n";
    }
    outfile.close();
}