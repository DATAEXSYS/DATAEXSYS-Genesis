Great — here is the complete **`docs/api.md`** file for Babe, including:

* **LDK SDK API**
* **NaaS Control API**
* **Policy API**
* **Event API**
* **Configuration API**
* **Examples and formats**

---

# ✅ `docs/api.md`

---

# **Babe API & Interfaces**

## **Purpose**

This document defines the public APIs and interfaces for:

* **Babe LDK (SDK APIs)**
* **Babe NaaS (Control Plane APIs)**
* **Policy & Compliance APIs**
* **Event APIs**
* **Configuration APIs**

---

# 1. API Design Principles

### 1.1 Principles

* **High performance**
* **Zero-copy**
* **Event-driven**
* **Minimal overhead**
* **Deterministic**
* **Backward compatible**

### 1.2 Interface Types

* **C/C++ SDK APIs (LDK)**
* **gRPC APIs (NaaS control plane)**
* **REST APIs (NaaS external integration)**
* **Event streaming APIs (NATS/Kafka)**

---

# 2. Babe LDK SDK API

## 2.1 Core Types

### Buffer Types

* `BufferPool`
* `BufferHandle`
* `BufferView`

### Event Types

* `NetEvent`
* `RouteEvent`
* `LinkEvent`
* `AppEvent`
* `TimerEvent`

---

## 2.2 Event API (LDK)

### Subscribe to events

```cpp
babe::EventBus bus;

bus.subscribe<NetEvent>([](const NetEvent &ev){
   // packet received
});

bus.subscribe<RouteEvent>([](const RouteEvent &ev){
   // route discovered or broken
});
```

### Emit events

```cpp
NetEvent ev;
bus.emit(ev);
```

---

## 2.3 Buffer API

### Allocate buffer

```cpp
auto buf = babe::BufferPool::alloc(size);
```

### Get view

```cpp
auto view = buf.view();
```

### Send packet

```cpp
babe::send(view, dest);
```

---

## 2.4 Routing API

### Discover route

```cpp
auto route = babe::Routing::discover(dest);
```

### Get cached route

```cpp
auto route = babe::Routing::getRoute(dest);
```

### Subscribe to route events

```cpp
bus.subscribe<RouteEvent>(...);
```

---

## 2.5 Transport API

### Send reliable

```cpp
babe::Transport::sendReliable(dest, buf);
```

### Send unreliable

```cpp
babe::Transport::sendUnreliable(dest, buf);
```

### Receive

```cpp
auto msg = babe::Transport::recv();
```

---

# 3. Babe NaaS Control API

## 3.1 gRPC Endpoints

### Node Registration

```
rpc RegisterNode(RegisterNodeRequest) returns (RegisterNodeResponse);
```

### Policy Distribution

```
rpc GetPolicyBundle(GetPolicyRequest) returns (PolicyBundle);
```

### Telemetry Ingestion

```
rpc IngestTelemetry(TelemetryStream) returns (TelemetryAck);
```

### Service Catalog

```
rpc GetServices(GetServicesRequest) returns (ServiceList);
```

### Compliance Reports

```
rpc GetComplianceReport(GetComplianceRequest) returns (ComplianceReport);
```

---

## 3.2 REST Endpoints (External Integration)

### Example

* `POST /api/v1/nodes/register`
* `GET /api/v1/policies`
* `POST /api/v1/telemetry`
* `GET /api/v1/services`
* `GET /api/v1/compliance`

---

# 4. Policy API

## 4.1 Policy Bundle Format

Policies are distributed as **versioned bundles**:

```yaml
policy_bundle:
  version: "v1.0.0"
  policies:
    - id: "policy_1"
      type: "routing"
      rules:
        - match: "dest==0x1234"
          action: "allow"
```

## 4.2 Policy Types

* Routing policies
* Security policies
* QoS policies
* Compliance policies
* Access control policies

---

# 5. Event Streaming API

## 5.1 Event Bus (NATS/Kafka)

Events are published by nodes and consumed by NaaS.

### Example Topics

* `babe.node.events`
* `babe.routing.events`
* `babe.policy.events`
* `babe.compliance.events`
* `babe.telemetry.events`

---

# 6. Configuration API

## 6.1 Configuration Format

Configuration is done via YAML/JSON:

```yaml
mode: ldk
network:
  interface: eth0
  protocol: dsr
performance:
  use_af_xdp: true
```

## 6.2 Configuration Sources

* Config file
* Environment variables
* CLI flags

---

# 7. Example: Node Startup CLI

```
babe --mode=ldk --config=/etc/babe/config.yaml
```

### Example: NaaS agent startup

```
babe --mode=naas-agent --config=/etc/babe/agent.yaml
```

---

# 8. Versioning & Compatibility

### 8.1 API Versioning

* `v1`, `v2`, `v3`
* Backward compatible
* Deprecation notices

### 8.2 Semantic Versioning

* MAJOR.MINOR.PATCH

---

# Next File

If you approve this, the next step is:

### **Generate the Helm chart skeleton + Kubernetes manifests**

Reply **“Next: Helm/K8s”** when you’re ready.
