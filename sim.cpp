#include "Blockchain/PKCertChain/Crypto.h"
#include "Blockchain/PKCertChain/Signkeys.h"
#include "Blockchain/PKCertChain/PKCertChain.h"
#include "Consensus/PoW.h"
#include "Helper/CSPRNG.h"

#include <iostream>
#include <vector>
#include <array>
#include <algorithm>

int main() {
    std::cout << "--- Testing PKCertChain Integration with Blockchain ---" << std::endl;

    // 1. Generate Node ID
    uint32_t myId = idGen();
    std::cout << "Generated Node ID: " << myId << std::endl;

    // 2. Generate Sign Keys (Ed25519)
    std::cout << "\n[Test] Generating Sign Keys..." << std::endl;
    if (!GenerateAndLogSignKeys(myId)) {
        std::cerr << "Sign Key Gen Failed!" << std::endl;
        return 1;
    }
    std::cout << "Sign Keys Generated." << std::endl;

    // Load public signing key
    EVP_PKEY* pubSign = LoadPublicKey(myId);
    std::vector<unsigned char> pkSignVec;
    if (pubSign) {
        unsigned char* der = nullptr;
        int len = i2d_PUBKEY(pubSign, &der);
        if (len > 0) {
            pkSignVec.assign(der, der + len);
            OPENSSL_free(der);
        }
        EVP_PKEY_free(pubSign);
    }

    // 3. Generate Encryption Keys (X25519)
    std::cout << "\n[Test] Generating Encryption Keys..." << std::endl;
    if (!GenerateAndLogX25519Key(myId)) {
        std::cerr << "Encryption Key Gen Failed!" << std::endl;
        return 1;
    }
    std::cout << "Encryption Keys Generated." << std::endl;

    // Load public encryption key
    EVP_PKEY* pubEnc = LoadPublicKey(myId);
    std::vector<unsigned char> pubEncVec;
    if (pubEnc) {
        unsigned char* der = nullptr;
        int len = i2d_PUBKEY(pubEnc, &der);
        if (len > 0) {
            pubEncVec.assign(der, der + len);
            OPENSSL_free(der);
        }
        EVP_PKEY_free(pubEnc);
    }

    // 4. Initialize Blockchain
    Blockchain* bc = new Blockchain();

    // 5. Prepare PoW challenge
    std::array<uint8_t, 32> pubKeyArray{};
    std::copy(pubEncVec.begin(), pubEncVec.begin() + std::min(pubEncVec.size(), pubKeyArray.size()), pubKeyArray.begin());
    std::array<uint8_t, 64> dummySig{}; // placeholder signature

    uint16_t shortId = static_cast<uint16_t>(myId);
    PowChallenge* challenge = GeneratePoWChallenge(nullptr, shortId, pubKeyArray, dummySig);

    uint64_t nonce = SolvePowChallenge(challenge);

    // 6. Create the Certificate block and add to blockchain
    Certificate* block = new Certificate();
    block->nodeID = myId;
    block->PKSign = pkSignVec;
    block->PubEncKey = pubEncVec;
    block->challenge = *challenge;
    block->nonce = nonce;

    bc->AddBlock(*block);

    // 7. Cleanup
    delete challenge;
    delete block;
    delete bc;

    std::cout << "\nBlock added to blockchain!" << std::endl;
    std::cout << "Node ID: " << myId 
              << " | Nonce: " << nonce 
              << " | Challenge[0]: " << int(block->challenge.challenge[0])
              << std::endl;

    std::cout << "\n--- PKCertChain + Blockchain Integration Test Completed ---" << std::endl;
    return 0;
}
