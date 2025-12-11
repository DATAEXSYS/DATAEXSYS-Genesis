#pragma once
#include <unordered_map>
#include <cstdint>
#include <iostream>
#include <iomanip>

class LocalTrustDiary {
public:

    struct TrustRecord {
        uint32_t nodeID;
        uint32_t Acks = 0;
        uint32_t Nacks = 0;
        double Trust = 0.5;  // Beta(1,1) prior mean

        // Alpha = 1 + ACKs
        double alpha() const { return 1.0 + Acks; }

        // Beta = 1 + NACKs
        double beta() const { return 1.0 + Nacks; }
    };

private:
    std::unordered_map<uint32_t, TrustRecord> diary;

    // Bayesian trust update
    void updateTrust(TrustRecord& rec) {
        double a = rec.alpha();
        double b = rec.beta();
        rec.Trust = a / (a + b);
    }

    // Create record if missing
    void registerNode(uint32_t id) {
        if (!diary.count(id)) {
            TrustRecord rec{};
            rec.nodeID = id;
            diary[id] = rec;
        }
    }

public:

    // Positive behavior
    void addAck(uint32_t id) {
        registerNode(id);
        TrustRecord& rec = diary[id];
        rec.Acks++;
        updateTrust(rec);
    }

    // Negative behavior
    void addNack(uint32_t id) {
        registerNode(id);
        TrustRecord& rec = diary[id];
        rec.Nacks++;
        updateTrust(rec);
    }

    // Read trust value (auto-register)
    double getTrust(uint32_t id) {
        registerNode(id);
        return diary[id].Trust;
    }

    // Pretty print
    void printDiary() const {
        std::cout << "\n===== Local Trust Diary =====\n";
        std::cout << std::setw(10) << "NodeID"
                  << std::setw(10) << "ACKs"
                  << std::setw(10) << "NACKs"
                  << std::setw(12) << "Trust"
                  << "\n------------------------------------------\n";

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
