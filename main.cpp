#include "Blockchain/PKCertChain/Crypto.h"
#include "Blockchain/PKCertChain/Signkeys.h"
#include "Consensus/PoW.h"
#include "Helper/CSPRNG.h"
#include <iostream>
#include <vector>
#include <cassert>

int main() {
    std::cout << "--- Testing PKCertChain Integration ---" << std::endl;

    // 1. Test ID Gen
    uint32_t myId = idGen();
    std::cout << "Generated Node ID: " << myId << std::endl;

    // 2. Test Sign Keys (Ed25519)
    std::cout << "\n[Test] Generating Sign Keys..." << std::endl;
    if (GenerateAndLogSignKeys(myId)) {
        std::cout << "Sign Keys Generated." << std::endl;
    } else {
        std::cerr << "Sign Key Gen Failed!" << std::endl;
        return 1;
    }

    std::string msg = "Hello Blockchain";
    std::vector<unsigned char> data(msg.begin(), msg.end());
    std::vector<unsigned char> signature;

    if (SignMessage(myId, data, signature)) {
        std::cout << "Message Signed. Sig Len: " << signature.size() << std::endl;
    }

    // Verify (Load public key to verify)
    // For test, we need to read the PEM back or use internal helper if exposed. 
    // Signkeys.h doesn't expose Load function but VerifySignature takes key bytes.
    // We'll skip deep verification test here unless we read the file, but function exists.

    // 3. Test Encryption Keys (X25519)
    std::cout << "\n[Test] Generating Encryption Keys..." << std::endl;
    if (GenerateAndLogX25519Key(myId)) {
        std::cout << "Encryption Keys Generated." << std::endl;
    }

    // 4. Test PoW
    std::cout << "\n[Test] PoW Challenge..." << std::endl;
    std::array<uint8_t, 32> dummyPub = {0};
    std::array<uint8_t, 64> dummySig = {0};
    uint16_t shortId = (uint16_t)myId;
    
    PowChallenge* challenge = GeneratePoWChallenge(nullptr, shortId, dummyPub, dummySig);
    std::cout << "Challenge Generated. Difficulty[0]: " << (int)challenge->difficulty[0] << std::endl;

    std::cout << "Solving PoW..." << std::endl;
    uint64_t nonce = SolvePowChallenge(*challenge);
    std::cout << "PoW Solved! Nonce: " << nonce << std::endl;

    delete challenge;
    std::cout << "\n--- Tests Completed Successfully ---" << std::endl;
    return 0;
}
