#include "Consensus/PoW.h"
#include <cstring>
#include <vector>

// Simple PoW implementation
// Valid if SHA256(challenge + nonce)[0] <= difficulty[0] AND SHA256(...)[1] <= difficulty[1]
// We assume difficulty is 2 bytes "Target" prefix.

PowChallenge* GeneratePoWChallenge(nodeReg* prevBlock, uint16_t& id, std::array<uint8_t, 32> publicKey, std::array<uint8_t, 64> signature) {
    PowChallenge* challenge = new PowChallenge();
    // 32 random bytes for challenge
    challenge->challenge.resize(32);
    if(RAND_bytes(challenge->challenge.data(), 32) != 1) {
        // Handle error or throw
    }

    // Set difficulty (For testing: First byte <= 0x10 is moderately easy)
    challenge->difficulty.resize(2);
    challenge->difficulty[0] = 0x10; 
    challenge->difficulty[1] = 0x00; 

    return challenge;
}

uint64_t SolvePowChallenge(PowChallenge& challenge) {
    uint64_t nonce = 0;
    std::vector<unsigned char> buffer;
    buffer.reserve(challenge.challenge.size() + sizeof(nonce));

    while (true) {
        buffer = challenge.challenge;
        // Append nonce (little-endian or raw bytes)
        for(int i=0; i<8; ++i) {
            buffer.push_back((nonce >> (i*8)) & 0xFF);
        }

        std::vector<unsigned char> hash = sha256(buffer);

        // Check difficulty (Target Check implementation)
        // Here we require Hash <= Difficulty (lexicographically for first 2 bytes)
        // Actually, let's keep it simple: Hash[0] MUST be <= difficulty[0].
        if (hash[0] <= challenge.difficulty[0]) {
             // Found!
             return nonce;
        }

        nonce++;
        if (nonce == UINT64_MAX) break; // Failed
    }
    return 0;
}
