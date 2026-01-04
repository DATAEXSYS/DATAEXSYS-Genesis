# BABE Engine Documentation

**BABE** – Baby Adhoc Base Engine  
Version: 0.1 (Genesis)  
Author: Salman 

---

## Table of Contents

1. [Introduction](#introduction)  
2. [Architecture Overview](#architecture-overview)  
3. [Getting Started](#getting-started)  
4. [SDK Documentation](#sdk-documentation)  
5. [Rust Tooling](#rust-tooling)  
6. [Testing and Simulation](#testing-and-simulation)  
7. [Best Practices](#best-practices)  
8. [Reference](#reference)  

---

## 1. Introduction

**BABE** (Baby Adhoc Base Engine) is a headless, modular networking engine designed to support ad-hoc decentralized applications such as **DATAEXSYS**.  

**Key Features:**
- A professional C++23 SDK for system-level ad-hoc network programming.
- Comprehensive Rust-based tooling for debugging, analysis, performance monitoring, and security checks.
- Modular architecture: App | Link | Trans | Net  
- Designed for heterogeneous networks (PCs, MCUs)  
- Provides FFI/IPC interfaces for observation and control  

**Purpose:**  
BABE acts as a foundational "bare-metal" engine, providing developers with the essential components to build and experiment with custom networking stacks. It is designed to serve as the runtime for advanced protocols such as the **Blockchain-Enhanced DSR** implemented in the DATAEXSYS project. It facilitates practical deployment, research, and network simulations via a layer-by-layer breakdown of its tooling and architecture, without embedding application-specific logic.  

---

## 2. Architecture Overview

BABE is organized into two main layers:

### 2.1 SDK (C++ Core)
- **App Module:** Node lifecycle, configuration management  
- **Link Module:** Peer discovery, session management  
- **Trans Module:** Transport layer (reliable/unreliable channels, UDP/QUIC)  
- **Net Module:**  
  - Control: Node sessions, authentication hooks  
  - Data: Reliable/unreliable messaging  

### 2.2 Rust Tooling
- Observes and interacts with BABE SDK via FFI or IPC  
- Provides a comprehensive suite for:
  - **Debugging & Analysis:** Deep packet inspection, event tracing, and replay.
  - **Performance:** Metrics collection, logging, and performance profiling.
  - **Simulation:** Multi-node orchestration and network condition modeling.
  - **Security:** Pluggable hooks for monitoring and security checks.  

### 2.3 Communication
- **FFI Interface:** Minimal C ABI for lifecycle control (`create_node`, `tick`, `destroy`)  
- **IPC / Shared Memory:** For logs, metrics, network events  

**Diagram Placeholder:**
```

[ BABE Engine ]
│
├─ SDK (C++): App | Link | Trans | Net
│       │
│       └─ C ABI
│
└─ Rust Tooling: logging, simulation, monitoring
```

---

## 3. Getting Started

### 3.1 Building the SDK
```bash
cd BABE/SDK
cmake -S . -B build
cmake --build build
````

### 3.2 Building Rust Tooling

```bash
cd BABE/Tools
cargo build
```

### 3.3 Running TestEngine

```bash
cd BABE/TestEngine
./node_main --config node1.toml
```

### 3.4 Hello Node Example

* Create a single node
* Tick it in a loop
* Observe logs via Rust tooling

---

## 4. SDK Documentation

### 4.1 App Module

* **Responsibilities:** Node lifecycle, configuration, initialization
* **Example API:**

```cpp
be_engine_t* be_engine_create(const be_config_t* config);
void be_engine_tick(be_engine_t* engine, double dt);
void be_engine_destroy(be_engine_t* engine);
```

### 4.2 Link Module

* Peer discovery, session management
* Maintain peer tables, handle dynamic joins/leaves

### 4.3 Trans Module

* Provides transport over UDP/QUIC
* Supports reliable and unreliable channels

### 4.4 Net Module

* **Control:** node sessions, minimal access hooks
* **Data:** sending/receiving messages, batching

### 4.5 FFI / C ABI

* Allows Rust tooling to control SDK nodes
* Observability hooks for metrics, events, and logs

---

## 5. Rust Tooling

### 5.1 Tooling Modules

* Mirrors SDK structure: app, link, trans, net
* Provides observability without modifying SDK behavior

### 5.2 Key Features

* **Debugging & Analysis:** Deep packet inspection, event tracing, and replay.
* **Performance:** Metrics collection, logging, and performance profiling.
* **Simulation:** Multi-node orchestration and network condition modeling.
* **Security:** Pluggable hooks for monitoring and security checks for applications like DATAEXSYS.

### 5.3 Example Rust FFI Usage

```rust
let mut engine = EngineHandle::new(config);
engine.tick(0.016); // tick node
engine.subscribe_logs(|log_event| println!("{:?}", log_event));
```

---

## 6. Testing and Simulation

### 6.1 Unit Testing

* `TestSDK` for module-level tests
* Validate message delivery, transport reliability

### 6.2 Network Simulation

* `TestEngine` allows multiple nodes to run on one host
* Heterogeneous simulation (PCs + MCU nodes)
* Logging and monitoring via Rust tooling

---

## 7. Best Practices

* Keep SDK minimal and modular
* Avoid application-specific logic in BABE
* Use Rust tooling for analysis, not core functionality
* Use FFI/IPC for safe, controlled interaction
* Add new transports as modular extensions

---

## 8. Reference

* **C++ SDK APIs**: lifecycle, networking, messaging
* **Rust Tooling APIs**: logging, metrics, multi-node control
* **IPC / Shared Memory Formats**: events, logs, messages
* **Example Configurations**: node parameters, logging options

**End of BABE Engine Documentation**
