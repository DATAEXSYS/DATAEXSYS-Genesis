# DATAEXSYS-Genesis
### Decentralized Ad-hoc Transfer and Access Exchange System

**DATAEXSYS-Genesis** is a C++ based simulator for a secure data exchange system designed for Personal Ad-hoc Networks (PANETs). It implements a novel security framework on top of the Dynamic Source Routing (DSR) protocol to protect against common network attacks in a decentralized environment.

This `Genesis` variant focuses on simulating the core security and performance algorithms in a controlled, single-PC environment.

## Core Concepts

The system secures ad-hoc networks by integrating three key components built upon a multi-layer blockchain architecture:

1.  **Local Trust Diaries**: Each node maintains a trust score for its neighbors using a Bayesian inference model (Beta-Bernoulli). This allows nodes to make routing decisions based on the past behavior and reliability of their peers.
2.  **PKCertChain (PKI Blockchain)**: A dedicated blockchain layer that manages Public Key Infrastructure (PKI). It handles node registration, identity verification, and certificate issuance/renewal using a lightweight Proof-of-Work and a density-aware quorum-based consensus.
3.  **RouteLogChain (Routing Blockchain)**: A ledger that securely logs routing information. It validates paths using a multi-metric Dijkstra's algorithm and protects data in transit with a "rolling signature" mechanism to ensure integrity and mitigate attacks.

## Security Features

The protocol is designed to detect and mitigate several critical security threats:

-   **Sybil Attacks**: Prevented by requiring new nodes to pass a Proof-of-Work challenge and gain approval from a quorum of existing nodes.
-   **Blackhole Attacks**: Mitigated by the Local Trust Diaries, which quickly identify and isolate nodes that consistently drop packets.
-   **Wormhole Attacks**: Countered by the RouteLogChain's rolling signature mechanism, which makes it computationally infeasible to tunnel packets without being detected.
-   **Replay Attacks**: Defeated by timestamping and logging transactions on the blockchain, preventing stale or duplicate packets from being accepted.

## Technology Stack

-   **Language**: C++23
-   **Compiler**: GCC 11 / Clang 14
-   **Concurrency**: POSIX Threads (`<thread>`), `<mutex>`, `<atomic>`
-   **Networking**: POSIX Sockets for simulated node-to-node communication.
-   **Build System**: CMake

## Build and Run

This project uses CMake to manage the build process.

```bash
# 1. Clone the repository
git clone <repository-url>
cd DATAEXSYS-Genesis

# 2. Configure the build using CMake
mkdir build && cd build
cmake ..

# 3. Compile the project
make

# 4. Run the simulator
./dataexsys_simulator
```