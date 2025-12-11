// sim.cpp
#include "Blockchain/PKCertChain/Crypto.h"
#include "Blockchain/PKCertChain/Signkeys.h"
#include "Blockchain/PKCertChain/PKCertChain.h"
#include "Consensus/PoW.h"
#include "Helper/CSPRNG.h"
#include "Trust/LocalTrustDiary.h"

#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

// Helper: read whole file into vector<unsigned char>
static std::vector<unsigned char> ReadFileBytes(const fs::path& p) {
    std::vector<unsigned char> out;
    std::ifstream in(p, std::ios::binary);
    if (!in.is_open()) return out;
    in.seekg(0, std::ios::end);
    std::streamsize size = in.tellg();
    if (size <= 0) return out;
    in.seekg(0, std::ios::beg);
    out.resize((size_t)size);
    if (!in.read(reinterpret_cast<char*>(out.data()), size)) {
        out.clear();
    }
    return out;
}

int main() {
    std::cout << "--- PKCertChain Blockchain + Crypto Test ---" << std::endl;

    // 1) Generate Node ID
    uint32_t myId = idGen();
    std::cout << "Generated Node ID: " << myId << std::endl;

    // 2) Generate Sign Keys (Ed25519)
    std::cout << "\n[Step] Generating Ed25519 signing keys..." << std::endl;
    if (!GenerateAndLogSignKeys(myId)) {
        std::cerr << "Sign key generation failed." << std::endl;
        return 1;
    }

    // 3) Generate Encryption Keys (X25519)
    std::cout << "\n[Step] Generating X25519 encryption keys..." << std::endl;
    if (!GenerateAndLogX25519Key(myId)) {
        std::cerr << "Encryption key generation failed." << std::endl;
        return 1;
    }

    // 4) Load public keys (must be after both key generations)
    //    LoadPublicKey returns EVP_PKEY* (caller must free)
    EVP_PKEY* pubSign = LoadPublicKey(myId); // Ed25519 public
    if (!pubSign) {
        std::cerr << "Failed to load public signing key for id " << myId << std::endl;
        return 1;
    }
    EVP_PKEY* pubEnc = LoadPublicKey(myId);  // X25519 public (same loader)
    if (!pubEnc) {
        std::cerr << "Failed to load public encryption key for id " << myId << std::endl;
        EVP_PKEY_free(pubSign);
        return 1;
    }

    // 5) Serialize public keys to DER bytes for storing in Certificate
    std::vector<unsigned char> pkSignVec;
    {
        unsigned char* der = nullptr;
        int len = i2d_PUBKEY(pubSign, &der);
        if (len > 0 && der) {
            pkSignVec.assign(der, der + len);
            OPENSSL_free(der);
        }
    }

    std::vector<unsigned char> pubEncVec;
    {
        unsigned char* der = nullptr;
        int len = i2d_PUBKEY(pubEnc, &der);
        if (len > 0 && der) {
            pubEncVec.assign(der, der + len);
            OPENSSL_free(der);
        }
    }

    // 6) Initialize blockchain (creates its log dir/file in current_path)
    Blockchain* bc = new Blockchain();

    // Print current working dir so you know where BlockLogs will be created
    std::cout << "Current working directory: " << fs::current_path() << std::endl;

    // 7) Create PoW challenge and solve (use first 32 bytes of pubEncVec as pubKey)
    std::array<uint8_t, 32> pubKeyArray{};
    {
        size_t copySize = std::min(pubEncVec.size(), pubKeyArray.size());
        std::copy(pubEncVec.begin(), pubEncVec.begin() + copySize, pubKeyArray.begin());
    }
    std::array<uint8_t, 64> dummySig{}; // placeholder
    uint16_t shortId = static_cast<uint16_t>(myId);
    PowChallenge* challenge = GeneratePoWChallenge(nullptr, shortId, pubKeyArray, dummySig);
    if (!challenge) {
        std::cerr << "Failed to generate PoW challenge." << std::endl;
        EVP_PKEY_free(pubSign);
        EVP_PKEY_free(pubEnc);
        delete bc;
        return 1;
    }
    std::cout << "[PoW] Solving challenge (difficulty[0] = " << int(challenge->difficulty[0]) << ")..." << std::endl;
    uint64_t nonce = SolvePowChallenge(*challenge);
    std::cout << "[PoW] Solved. Nonce = " << nonce << std::endl;

    // 8) Create Certificate and add to blockchain
    Certificate* block = new Certificate();
    block->nodeID = myId;
    block->PKSign = pkSignVec;
    block->PubEncKey = pubEncVec;
    block->challenge = *challenge; // copy
    block->nonce = nonce;

    bc->AddBlock(*block);
    std::cout << "[Blockchain] Block added (node id = " << block->nodeID << ")." << std::endl;

    // 9) Prepare a message and sign it (SignMessage uses private key file internally)
    std::string message = "Hello, DATAEXSYS Blockchain!";
    std::vector<unsigned char> msgVec(message.begin(), message.end());
    std::vector<unsigned char> signature;
    if (!SignMessage(myId, msgVec, signature)) {
        std::cerr << "SignMessage failed." << std::endl;
        // cleanup
        delete challenge;
        delete block;
        delete bc;
        EVP_PKEY_free(pubSign);
        EVP_PKEY_free(pubEnc);
        return 1;
    }
    std::cout << "[Sign] Message signed. Signature length = " << signature.size() << std::endl;

    // 10) Encrypt message using X25519-derived shared secret + AES-GCM
    std::vector<unsigned char> ciphertext, iv, tag;
    if (!Encrypt(myId, pubEnc, msgVec, ciphertext, iv, tag)) {
        std::cerr << "Encrypt failed." << std::endl;
        // cleanup
        delete challenge;
        delete block;
        delete bc;
        EVP_PKEY_free(pubSign);
        EVP_PKEY_free(pubEnc);
        return 1;
    }
    std::cout << "[Encrypt] Ciphertext length = " << ciphertext.size() << ", IV len = " << iv.size()
              << ", TAG len = " << tag.size() << std::endl;

    // 11) Decrypt (verify correctness)
    std::vector<unsigned char> plaintext;
    if (!Decrypt(myId, pubEnc, ciphertext, iv, tag, plaintext)) {
        std::cerr << "Decrypt failed or authentication failed." << std::endl;
    } else {
        std::string recovered(plaintext.begin(), plaintext.end());
        std::cout << "[Decrypt] Recovered plaintext: " << recovered << std::endl;
    }

    // 12) Verify signature using public signing key PEM bytes (read PEM file)
    //    Read public signing PEM file from disk (SignKeys/<id>_public.pem)
    fs::path signPemPath = fs::current_path() / "SignKeys" / (std::to_string(myId) + "_public.pem");
    auto pubSignPemBytes = ReadFileBytes(signPemPath);
    if (pubSignPemBytes.empty()) {
        std::cerr << "Failed to read public signing PEM: " << signPemPath << std::endl;
    } else {
        bool ok = VerifySignature(pubSignPemBytes, msgVec, signature);
        std::cout << "[Verify] Signature verification: " << (ok ? "SUCCESS" : "FAIL") << std::endl;
    }

    // 13) Write a node-level log (BlockLogs/node_log.txt) with details
    fs::path logDir = fs::current_path() / "BlockLogs";
    if (!fs::exists(logDir)) {
        if (!fs::create_directories(logDir)) {
            std::cerr << "Failed to create log directory: " << logDir << std::endl;
        }
    }
    fs::path logFile = logDir / "node_log.txt";
    std::ofstream logOut(logFile.string(), std::ios::app);
    if (!logOut.is_open()) {
        std::cerr << "Failed to open log file: " << logFile << std::endl;
    } else {
        logOut << "Node ID: " << block->nodeID
               << " | PKSign(DER) size: " << block->PKSign.size()
               << " | PubEnc(DER) size: " << block->PubEncKey.size()
               << " | Nonce: " << block->nonce
               << " | SigLen: " << signature.size()
               << " | CipherLen: " << ciphertext.size()
               << std::endl;
        logOut.close();
        std::cout << "[Log] Wrote node info to: " << logFile << std::endl;
    }

    // 14) Cleanup all raw pointers
    EVP_PKEY_free(pubSign);
    EVP_PKEY_free(pubEnc);
    delete challenge;
    delete block;
    delete bc;

    std::cout << "\nAll done — test completed successfully." << std::endl;
    
    LocalTrustDiary trust;

// Simulate some neighbors
uint32_t N1 = 1001;
uint32_t N2 = 2002;

// Behavior updates
trust.addAck(N1);    // N1 behaved correctly
trust.addAck(N1);
trust.addNack(N2);   // N2 misbehaved
trust.addAck(N2);

// Print results
trust.printDiary();
    
}
