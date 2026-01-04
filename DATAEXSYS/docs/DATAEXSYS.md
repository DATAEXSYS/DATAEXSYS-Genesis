# DATAEXSYS System Documentation

**DATAEXSYS** – Decentralized Adhoc Transfer and Access Exchange System  
Version: 0.1 (Genesis)  
Author: Salman  

---

## Table of Contents

1. [Introduction](#introduction)  
2. [System Architecture](#system-architecture)  
3. [Components Overview](#components-overview)  
4. [Getting Started](#getting-started)  
5. [Configuration](#configuration)  
6. [Usage Guide](#usage-guide)  
7. [Security Mechanisms](#security-mechanisms)  
8. [Threat Mitigations](#threat-mitigations)  
9. [Blockchain Integration](#blockchain-integration)  
10. [Frontend / GUI](#frontend--gui)  
11. [Examples / Tutorials](#examples--tutorials)  
12. [Best Practices](#best-practices)  
13. [Reference](#reference)  

---

## 1. Introduction

**DATAEXSYS** is a fully decentralized ad-hoc network **system** designed to operate across heterogeneous nodes (PCs, MCUs). Its official academic title is **Secure Data Exchange System for Ad-hoc Networks Using Blockchain-Enhanced DSR**.

**Key Features:**
- Implements the **Advanced Blockchain Dynamic Source Routing (ABCD)** protocol.
- Secure communication using a decentralized PKI and rolling digital signatures.
- Immutable logging and identity management via a multi-layer blockchain.
- Proactive threat mitigation against Blackhole, Wormhole, Sybil, and Replay attacks using **Bayesian Trust Diaries**.
- Uses the **BABE engine** for the underlying networking runtime and tooling.

**Purpose:**  
To provide a research-grade platform for secure, decentralized ad-hoc networking experiments. It offers a full system interface (backend + frontend) for developers and researchers, implementing the novel security features of the Blockchain-Enhanced DSR protocol.

---

## 2. System Architecture

DATAEXSYS implements the **Advanced Blockchain Dynamic Source Routing (ABCD)** protocol, which features a multi-layer architecture:

```
[Frontend GUI] -> [Backend Services] -> [DATAEXSYS Security Layer] -> [BABE Engine]
```

- **Frontend GUI:** User interface for monitoring, visualization, and network analysis.
- **Backend Services:** Orchestrates the DATAEXSYS security layer and interfaces with the BABE engine.
- **DATAEXSYS Security Layer:** The core of the system, which includes:
  - **Multi-Layer Blockchain:** `PKCertChain` for identity and `RouteLogChain` for path logging.
  - **Trust Management:** `Bayesian Trust Diaries` to evaluate node reputation.
  - **Secure Routing:** A hybrid proactive-reactive routing mechanism based on DSR.
- **BABE Engine:** Provides the foundational, low-level ad-hoc networking SDK (peer discovery, transport, messaging).

---

## 3. Components Overview

### 3.1 Frontend (GUI)
- Displays network topology, node health, and security events.
- Provides an interface for configuring and managing nodes.

### 3.2 Backend Services
- Manages the node lifecycle and orchestrates communication via the BABE engine.
- Enforces security policies defined by the DATAEXSYS layer.
- Interfaces with the blockchain for logging and identity verification.

### 3.3 DATAEXSYS Security Layer
- **PKCertChain:** A blockchain layer for decentralized Public Key Infrastructure. Manages node registration, certificate issuance, and authentication using a lightweight PoW and quorum-based consensus.
- **RouteLogChain:** A blockchain layer that securely logs routing paths. It works with a multi-metric Dijkstra's algorithm to ensure optimal and trustworthy routes.
- **Bayesian Trust Diaries:** A decentralized reputation system where each node locally assesses the behavior of its peers to calculate a trust score.
- **Secure DSR Protocol:** The enhanced routing protocol that integrates trust scores and blockchain verification into the route discovery process.  

---

## 4. Getting Started

### 4.1 Prerequisites
- BABE SDK installed  
- Rust tooling available  
- GUI dependencies (Qt / GTK / Web frontend)  

### 4.2 Build System
```bash
cd DATAEXSYS/backend
mkdir build && cd build
cmake ..
cmake --build .
````

```bash
cd DATAEXSYS/frontend
npm install && npm run build  # if web-based
```

### 4.3 Running the System

```bash
./dataexsys_backend --config ../config/node1.toml
./dataexsys_frontend  # Launch GUI
```

---

## 5. Configuration

* Node configuration: peer list, transport options, blockchain keys
* Security parameters: AES-GCM keys, replay protection settings
* GUI preferences: network visualization, logging verbosity

---

## 6. Usage Guide

* Start backend nodes
* Launch GUI for monitoring
* Send and receive messages across nodes
* Observe security alerts and blockchain logs via GUI

---

## 7. Security Mechanisms

The security of DATAEXSYS is built on a combination of decentralized trust, cryptographic authentication, and immutable logging.

*   **Decentralized PKI (`PKCertChain`):** Nodes are authenticated via a blockchain-based Public Key Infrastructure. Certificates are issued after a new node completes a lightweight Proof-of-Work challenge and is vouched for by a quorum of existing nodes.
*   **Bayesian Trust Diaries:** Each node maintains a local, private diary of its interactions with peers. It uses a Bayesian/Beta-Bernoulli model to update a trust score for each neighbor, allowing it to dynamically assess reputation.
*   **Rolling Digital Signatures:** Data packets are secured during transmission using rolling signatures. Each intermediate node signs over the previously signed packet, creating a verifiable chain of custody and protecting against in-transit modification.

---

## 8. Threat Mitigations

The system is designed to proactively defend against common ad-hoc network attacks using its unique architecture.

*   **Sybil Attack:** Mitigated by the `PKCertChain`'s consensus mechanism. An attacker cannot easily create numerous fake identities because each one must pass both a computational challenge (PoW) and a social consensus check (quorum).
*   **Blackhole & Wormhole Attacks:** Mitigated by the `RouteLogChain` and rolling signatures. The `RouteLogChain` provides a trusted record of paths, while rolling signatures ensure that any deviation or packet dropping by a malicious intermediate node breaks the signature chain and is immediately detectable.
*   **Replay Attack:** Mitigated by logging packet details in the `RouteLogChain`. The system can detect and drop duplicated packets that have already been processed and recorded on the blockchain.

---

## 9. Blockchain Integration

DATAEXSYS uses a specialized multi-layer blockchain architecture, moving beyond a monolithic design for efficiency and scalability.

*   **PKCertChain (Identity Layer):** This blockchain serves as a decentralized, tamper-proof registry for node identities and public key certificates. It is the foundation of the system's trust model.
*   **RouteLogChain (Operational Layer):** This chain records validated routing paths and can be used to log significant network events. It ensures that routing decisions are based on a secure, historical record.
*   **Meta Blockchain (Auditing Layer):** A higher-level chain that acts as a dispatcher and auditor, providing a single point of reference for the state of the other two chains.

---

## 10. Frontend / GUI

* Network topology visualization
* Node status dashboard
* Security alerts panel
* Blockchain ledger viewer

---

## 11. Examples / Tutorials

* Single-node simulation
* Multi-node heterogeneous network
* Threat simulation and monitoring via GUI

---

## 12. Best Practices

* Separate configs for heterogeneous nodes
* Monitor AES-GCM key rotation
* Sync blockchain regularly
* Use Rust tooling for research metrics

---

## 13. Reference

* Node APIs / CLI Commands
* Configuration file parameters
* Data formats: messages, events, blockchain transactions
* Security Standards: AES-GCM, blockchain audit logs

**End of DATAEXSYS System Documentation**
