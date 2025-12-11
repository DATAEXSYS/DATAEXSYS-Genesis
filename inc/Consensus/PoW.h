#pragma once
#include <iostream>
#include <stdint.h>
#include <array>
#include "Blockchain/PKCertChain/nodeReg.h"
#include "Helper/Serialize.h"
#include "Helper/Hash.h"
#include "Helper/CSPRNG.h"
#include "Consensus/PoWchallenge.h"

//----------------helper function ---------------------------

bool isSolved(const PowChallenge& chal, uint64_t nonce)
{
    // 1) Hash the nonce: SHA256(N)
    vector<uint8_t> nonce_bytes = BE64(nonce);
    vector<uint8_t> nonce_hash  = sha256(nonce_bytes);

    // 2) Combine R || SHA256(N)
    vector<uint8_t> input = chal.challenge;   // copy R (32 bytes)
    input.insert(input.end(),
                 nonce_hash.begin(),
                 nonce_hash.end());

    // 3) Hash again: SHA256(R || SHA256(N))
    vector<uint8_t> finalHash = sha256(input);

    // 4) Check prefix against T (2 bytes)
    return finalHash[0] == chal.difficulty[0] &&
           finalHash[1] == chal.difficulty[1];
}

//---------------------------------------------------------


uint64_t SolvePowChallenge(PowChallenge& challenge)
{
    const uint64_t MAX_ATTEMPTS = 1'000'000ULL;
    const unsigned int THREADS = std::thread::hardware_concurrency();

    std::atomic<bool> solved(false);
    std::atomic<uint64_t> solution(0);

    std::vector<std::jthread> pool;
    pool.reserve(THREADS);

    for (unsigned int i = 0; i < THREADS; ++i)
    {
        pool.emplace_back([&, i](std::stop_token st)
        {
            uint64_t nonce = CSPRNG64_t() + i;
            uint64_t step  = 0;

            while (!st.stop_requested() &&
                   !solved &&
                   step < MAX_ATTEMPTS)
            {
                if (isSolved(challenge, nonce))
                {
                    if (!solved.exchange(true))
                    {
                        solution = nonce;

                        // Stop ALL other threads immediately
                        for (auto& t : pool)
                            t.request_stop();
                    }

                    return;
                }

                nonce += THREADS;
                step++;
            }
        });
    }

    // jthread auto-joins on destruction
    if (!solved)
        throw std::runtime_error("PoW not solved");

    return solution.load();
}