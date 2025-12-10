#pragma once
#include <iostream>
#include <stdint.h>
#include <array>
#include "Blockchain/PKCertChain/nodeReg.h"
#include "Helper/Serialize.h"
#include "Helper/Hash.h"
#include "Helper/CSPRNG.h"
#include "Consensus/PoWchallenge.h"

using namespace std;

PowChallenge* GeneratePoWChallenge(nodeReg* prevBlock, uint16_t& id, std::array<uint8_t, 32> publicKey, std::array<uint8_t, 64> signature);
uint64_t SolvePowChallenge(PowChallenge& challenge);
