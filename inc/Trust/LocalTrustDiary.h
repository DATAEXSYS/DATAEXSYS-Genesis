#pragma once
#include <unordered_map>
#include <cstdint>
#include <iostream>
#include <iomanip>

struct TrustRecord {
    uint32_t nodeID = 0;
    uint32_t Acks = 0;
    uint32_t Nacks = 0;
    double Trust = 0.5; // initial Beta(1,1)

    double alpha() const { return 1.0 + Acks; }
    double beta() const { return 1.0 + Nacks; }
};

class LocalTrustDiary {
private:
    std::unordered_map<uint32_t, TrustRecord> diary;
    void registerNode(uint32_t id) {
        if (!diary.count(id)) diary[id] = TrustRecord{ id, 0, 0, 0.5 };
    }
    void updateTrust(TrustRecord& rec) {
        rec.Trust = rec.alpha() / (rec.alpha() + rec.beta());
    }
public:
    void addAck(uint32_t id) { registerNode(id); auto& r = diary[id]; r.Acks++; updateTrust(r); }
    void addNack(uint32_t id) { registerNode(id); auto& r = diary[id]; r.Nacks++; updateTrust(r); }
    double getTrust(uint32_t id) { registerNode(id); return diary[id].Trust; }

    void printDiary() const {
        std::cout << "\n===== Local Trust Diary =====\n";
        std::cout << std::setw(10) << "NodeID"
                  << std::setw(10) << "ACKs"
                  << std::setw(10) << "NACKs"
                  << std::setw(12) << "Trust" << "\n";
        std::cout << "------------------------------------------\n";
        for (const auto& [id, rec] : diary) {
            std::cout << std::setw(10) << rec.nodeID
                      << std::setw(10) << rec.Acks
                      << std::setw(10) << rec.Nacks
                      << std::setw(12) << std::fixed << std::setprecision(4)
                      << rec.Trust << "\n";
        }
        std::cout << "===========================================\n";
    }
};
