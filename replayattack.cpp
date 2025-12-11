#include "Node/DSRNode.h"
#include "Packet/Packet.h"
#include "Serialize/Serialize.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <cstdlib>

void clean_up() {
    std::filesystem::remove_all("test_data");
}

int main() {
    clean_up();
    std::filesystem::create_directory("test_data");
    std::filesystem::create_directory("test_data/node_1");
    std::filesystem::create_directory("test_data/node_2");
    std::filesystem::create_directory("test_data/node_3");

    // Setup neighbors
    { std::ofstream f("test_data/node_1/AccessTable.txt"); f << "2"; }
    { std::ofstream f("test_data/node_2/AccessTable.txt"); f << "1\n3"; }
    { std::ofstream f("test_data/node_3/AccessTable.txt"); f << "2"; }

    // DSRNodes
    DSRNode node1(1, 8001, 9001, 0);
    DSRNode node2(2, 8002, 9002, 0);
    DSRNode node3(3, 8003, 9003, 0);

    node1.set_node_dir("test_data/node_1");
    node2.set_node_dir("test_data/node_2");
    node3.set_node_dir("test_data/node_3");

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "[Test] Starting Replay Attack Simulation..." << std::endl;

    // 1. Construct a Valid DATA Packet manually using "Encrypted" payload simulation
    Packet p;
    p.type = PacketType::DATA;
    p.source_id = 1;
    p.destination_id = 3;
    p.sequence_number = 777; // Fixed Sequence Number
    p.timestamp = (uint32_t)std::time(nullptr);
    p.hopAddresses = {1, 2, 3}; // Valid Path
    p.hopcount = 3;

    std::string encrypted_payload = "ENCRYPTED_DATA_BLOB_XY92"; // Simulated crypto payload
    p.payload.assign(encrypted_payload.begin(), encrypted_payload.end());

    auto bytes = serialize_packet(p);

    // 2. Send ORIGINAL Packet
    std::cout << "[Test] Sending ORIGINAL Packet (Seq 777)..." << std::endl;
    // Node 1 sends to Node 2
    node1.send_packet(2, bytes);

    // Wait for processing
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 3. Send REPLAY Packet (Exact same bytes)
    std::cout << "!!!" << std::endl;
    std::cout << "[Test] Launching REPLAY ATTACK (Resending Seq 777)..." << std::endl;
    std::cout << "!!!" << std::endl;
    node1.send_packet(2, bytes);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "[Test] Simulation Finished. Check logs for 'REPLAY ATTACK DETECTED'." << std::endl;

    return 0;
}
