#include "Blockchain/PKCertChain/Crypto.h"
#include "Blockchain/PKCertChain/Signkeys.h"
#include "Blockchain/PKCertChain/PKCertChain.h"
#include "Blockchain/RouteLogChain/RouteLogChain.h"
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

int main() {
    std::cout << "--- PKCertChain + RouteLogChain + Trust Test ---\n";

    uint32_t myId = idGen();
    std::cout << "Node ID: " << myId << "\n";

    GenerateAndLogSignKeys(myId);
    GenerateAndLogX25519Key(myId);

    EVP_PKEY* pubSign = LoadPublicKey(myId);
    EVP_PKEY* pubEnc  = LoadPublicKey(myId);

    std::vector<unsigned char> pkSignVec(pubSign ? 32 : 0);
    std::vector<unsigned char> pubEncVec(pubEnc ? 32 : 0);

    Blockchain bc;
    uint16_t shortId = static_cast<uint16_t>(myId);
    std::array<uint8_t,32> pubKeyArray{};
    std::array<uint8_t,64> dummySig{};
    PowChallenge* challenge = GeneratePoWChallenge(nullptr, shortId, pubKeyArray, dummySig);
    uint64_t nonce = SolvePowChallenge(*challenge);
    delete challenge;

    Certificate block{};
    block.nodeID = myId;
    bc.AddBlock(block);

    LocalTrustDiary trust;
    trust.addAck(1);
    trust.addAck(2);
    trust.addNack(3);

    std::vector<std::vector<uint32_t>> network(5);
    network[0] = {1,2}; network[1]={0,2,3};
    network[2]={0,1,3,4}; network[3]={1,2,4}; network[4]={2,3};

    RouteLogChain rlc;
    auto paths = simulateRREQ(0,4,network);
    buildRouteBlocks(0,4,paths,trust,rlc);

    std::cout << "All route blocks:\n";
    for (auto& b : rlc.getChain()) {
        std::cout << "Src:" << b.source << " Dst:" << b.destination
                  << " Score:" << b.score << " Hops:";
        for (auto h : b.hopVector) std::cout << " " << h;
        std::cout << "\n";
    }

    trust.printDiary();
}
