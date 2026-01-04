# DATAEXSYS-Genesis: A Secure Ad-hoc Networking System

This repository contains the **DATAEXSYS-Genesis** project, a research initiative focused on building a secure, decentralized ad-hoc networking system. The project is the basis for a Final Year Project (FYP) and is composed of two primary components: the **DATAEXSYS** security system and the **BABE** networking engine.

---

## Table of Contents

- [Project Overview](#project-overview)
- [Core Components](#core-components)
  - [DATAEXSYS](#dataexsys)
  - [BABE (Baby Adhoc Base Engine)](#babe-baby-adhoc-base-engine)
- [Architecture](#architecture)
- [Getting Started](#getting-started)
- [Directory Structure](#directory-structure)

---

## Project Overview

DATAEXSYS-Genesis is a complete system for secure data exchange in ad-hoc networks. It implements a novel **Blockchain-Enhanced DSR (Dynamic Source Routing)** protocol to defend against common network attacks like Sybil, Blackhole, and Wormhole attacks.

The system is split into a high-level security and application layer (DATAEXSYS) and a low-level, foundational networking engine (BABE), promoting modularity and clean separation of concerns.

## Core Components

### DATAEXSYS

**DATAEXSYS (Decentralized Adhoc Transfer and Access Exchange System)** is the core security and application framework. It implements the advanced security protocols described in the accompanying research paper.

- **Key Features:**
  - Implements the **Advanced Blockchain Dynamic Source Routing (ABCD)** protocol.
  - A multi-layer blockchain for decentralized identity (`PKCertChain`) and secure route logging (`RouteLogChain`).
  - A decentralized reputation system using **Bayesian Trust Diaries**.
  - Proactive mitigation against common ad-hoc network threats.

> For detailed information, see the [**DATAEXSYS System Documentation**](./DATAEXSYS/docs/DATAEXSYS.md).

### BABE (Baby Adhoc Base Engine)

**BABE** is the foundational, "bare-metal" networking engine that DATAEXSYS is built upon. It provides a minimal, high-performance C++23 SDK for creating and managing ad-hoc network nodes.

- **Key Features:**
  - A professional C++23 SDK for system-level ad-hoc network programming.
  - Comprehensive Rust-based tooling for debugging, simulation, and analysis.
  - A modular architecture (`App | Link | Trans | Net`) for maximum flexibility.

> For detailed information, see the [**BABE Engine Documentation**](./BABE/docs/BABE-Engine.md).

## Architecture

The system architecture is designed in layers, with DATAEXSYS providing the security intelligence and BABE providing the core network runtime.

```
+-------------------------------------------------+
|      DATAEXSYS (Security & Application Layer)   |
| (Blockchain, Trust Model, Secure Routing Logic) |
+-------------------------------------------------+
|        BABE (Networking Engine & Tooling)       |
|    (Peer Discovery, Transport, C++ SDK, Rust)   |
+-------------------------------------------------+
```

This layered approach allows the security protocols to be developed and tested independently of the underlying networking implementation.

## Getting Started

To build and run the system, you must build the components in order.

1.  **Build BABE:** First, build the networking engine. Instructions can be found in the `BABE/docs/BABE-Engine.md` file.
2.  **Build DATAEXSYS:** Once BABE is built, build the DATAEXSYS system. Instructions can be found in the `DATAEXSYS/docs/DATAEXSYS.md` file.

## Directory Structure

- **/BABE/**: Contains the source code, tooling, and documentation for the BABE networking engine.
- **/DATAEXSYS/**: Contains the source code and documentation for the DATAEXSYS security system.
- **/F25_079/**: Contains the LaTeX source code for the associated academic research paper.
- `README.md`: This file.
