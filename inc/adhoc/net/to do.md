
# 🔹 Control Plane (CP)

**Role:** Decide network behavior, manage state, enforce security.

---

### **Functional Responsibilities**

1. **Routing Management**

   * Maintain Route SLL (append-only, verifiable)
   * Maintain Neighbor SLL
   * Look up routes for outgoing packets
   * Handle route discovery (RREQ, RREP)
   * Handle route errors (RERR)

2. **Security**

   * Verify incoming signatures (Ed25519)
   * Check replay windows
   * Manage session keys (X25519)
   * Prepare headers with crypto context

3. **Packet Preparation**

   * Decide packet type: DATA / RREQ / RREP / RERR / HELLO
   * Generate control metadata (source route, hop count, sequence numbers)
   * Sign / attach verification info
   * Validate incoming packets before writing state

4. **State Mutation (Gatekeeper)**

   * Append verified packets to SLL
   * Update route tables
   * Update neighbor tables
   * Trigger control-plane events (e.g., key rotation, route expiry)

---

### **Input / Output**

| Function                   | Input                | Output                                     |
| -------------------------- | -------------------- | ------------------------------------------ |
| `toPacket(state)`          | Internal SLL + event | Control packet (header, metadata)          |
| `fromPacket(packet_bytes)` | Received bytes       | Update SLL, routing tables, neighbor state |
| `lookupRoute(dst)`         | NodeID               | Route vector (or trigger RREQ)             |
| `verifyPacket(packet)`     | Incoming packet      | True/False for validity                    |

---

### **Key Rules**

* **All state writes go through CP**
* CP is **deterministic**: same inputs → same SLL outputs
* CP **never forwards payload**; it only prepares metadata
* CP **is the single source of truth** for network state

---

# 🔹 Data Plane (DP)

**Role:** Move payload efficiently according to CP’s decisions.

---

### **Functional Responsibilities**

1. **Payload Handling**

   * Accept transport layer payload
   * Encrypt payload (AES-GCM) if needed
   * Decrypt received payload

2. **Packet Forwarding**

   * Attach control plane headers
   * Use route info from CP to forward packets
   * Update forwarding counters (optional)
   * Deliver final payload to transport layer

3. **Next-Hop Execution**

   * Determine next hop from CP metadata
   * Push packet to link layer
   * Drop packets if invalid (without modifying SLL)

---

### **Input / Output**

| Function                                 | Input                          | Output                               |
| ---------------------------------------- | ------------------------------ | ------------------------------------ |
| `prepareDataPacket(payload, cp_headers)` | Transport payload + CP headers | Encrypted network packet bytes       |
| `forwardPacket(packet_bytes)`            | Packet + next hop              | Push to link layer                   |
| `decryptPacket(packet_bytes)`            | Encrypted network packet       | Decrypted payload to transport layer |

---

### **Key Rules**

* **DP never writes SLL**
* DP **executes CP decisions**
* DP can perform ephemeral state tracking (buffers, retransmissions)
* DP handles only **payload & forwarding**, not routing

---

# 🔹 Summary Table

| Feature         | Control Plane             | Data Plane                      |
| --------------- | ------------------------- | ------------------------------- |
| State mutation  | ✅ SLL, routing, neighbor  | ❌ Only ephemeral buffers        |
| Security        | ✅ Verify + sign + keys    | ✅ AES-GCM encryption/decryption |
| Routing         | ✅ Route discovery, update | ❌ Executes only forwarding      |
| Payload         | ❌ Not touched             | ✅ Encrypt, forward, decrypt     |
| Packet creation | ✅ Header + metadata       | ✅ Attach payload                |
| Forwarding      | ❌ Decide next hop         | ✅ Send to link                  |
| Determinism     | Must be deterministic     | Can be best-effort              |

---

# 🔹 Functional Flow (Send)

```
Application → Transport → Control Plane → Data Plane → Link → Hardware

1. Transport payload + destination
2. Control Plane: lookup route, prepare headers, sign
3. Data Plane: attach payload, encrypt, forward
4. Link Layer: frame packet
5. Hardware: transmit bits
```

---

# 🔹 Functional Flow (Receive)

```
Hardware → Link → Data Plane → Control Plane → Transport → Application

1. Hardware receives bits
2. Link decodes frames
3. Data Plane: decrypt payload, check next hop
4. Control Plane: verify signatures, append to SLL, update routes
5. Transport: reorder, reassemble
6. Application: deliver payload
```

---

✅ **Takeaway**

* **Control Plane = headers, routing, SLL, identity, state mutation**
* **Data Plane = payload, forwarding, encryption, execution**
* **All packets pass through CP first if they affect state**

