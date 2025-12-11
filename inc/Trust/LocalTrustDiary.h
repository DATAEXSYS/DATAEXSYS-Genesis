#pragma once
#include <cstdint>
#include <unordered_map>

struct TrustRecord {
    uint32_t nodeID;
    uint32_t Acks;   // successful interactions
    uint32_t Nacks;  // failures, misbehavior
    double   Trust;  // Bayesian trust score

    // α = Acks + 1, β = Nacks + 1
    double alpha() const { return Acks + 1.0; }
    double beta()  const { return Nacks + 1.0; }
};

class LocalTrustDiary {
private:
    std::unordered_map<uint32_t, TrustRecord> diary;

    // Bayesian update: Trust = α /(α + β)
    void updateTrust(TrustRecord& rec);

public:
    // Ensure record exists
    void registerNode(uint32_t id);

    // Call when a node behaves correctly
    void addAck(uint32_t id);

    // Call when a node misbehaves
    void addNack(uint32_t id);

    // Retrieve trust value
    double getTrust(uint32_t id);

    // Print trust diary
    void printDiary() const;
};
