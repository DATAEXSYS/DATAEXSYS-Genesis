#include "LocalTrustDiary.h"
#include <iostream>
#include <iomanip>

void LocalTrustDiary::registerNode(uint32_t id) {
    if (!diary.count(id)) {
        TrustRecord rec{};
        rec.nodeID = id;
        rec.Acks = 0;
        rec.Nacks = 0;
        rec.Trust = 0.5; // Beta(1,1) mean = 0.5
        diary[id] = rec;
    }
}

void LocalTrustDiary::updateTrust(TrustRecord& rec) {
    double a = rec.alpha();
    double b = rec.beta();
    rec.Trust = a / (a + b);
}

void LocalTrustDiary::addAck(uint32_t id) {
    registerNode(id);
    TrustRecord& rec = diary[id];
    rec.Acks++;
    updateTrust(rec);
}

void LocalTrustDiary::addNack(uint32_t id) {
    registerNode(id);
    TrustRecord& rec = diary[id];
    rec.Nacks++;
    updateTrust(rec);
}

double LocalTrustDiary::getTrust(uint32_t id) {
    registerNode(id);
    return diary[id].Trust;
}

void LocalTrustDiary::printDiary() const {
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
