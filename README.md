<<<<<<< HEAD
# 🌐 Decentralized Mesh Networking Stack

A radio-agnostic, decentralized mesh networking system for embedded devices — designed to run without any central server.

This project enables devices to **discover, communicate, and relay messages across a mesh network**, with initial validation on ESP-NOW and deployment targeting LoRa.

---

## 🧭 Purpose

The goal is to build a **modular mesh networking stack** that:

- Works across multiple radio technologies
- Requires **no central infrastructure**
- Supports **reliable multi-hop communication**

### Target Environments

| Environment | Radio | Purpose |
|------------|------|--------|
| Development | ESP-NOW | Fast iteration, short range |
| Deployment | LoRa | Long range, real-world usage |

### 🚫 Non-Goals (for now)

- Mobile apps
- Cloud dashboards
- Paid infrastructure/services

---

## ✅ Features

### Core Functionality

- 🔍 Node discovery & neighbor awareness  
- 🆔 Unique node addressing (ID + optional label)  
- 📡 Message types:
  - Broadcast
  - Unicast
  - Acknowledgements (ACKs)
- 🔁 Relay system:
  - TTL (time-to-live)
  - Duplicate suppression
- 🧭 Routing strategies (pluggable & evolving)
- 💾 Optional store-and-forward support

### Non-Functional Goals

- 🔌 Radio-agnostic architecture
- 🔋 Power-efficient operation (sleep/duty cycle)
- 📶 Reliable packet delivery over distance
- 📈 Scalable network size & diameter
- 🔐 Security baseline (authenticity, optional encryption)

---

## 🏗️ Architecture

The system is split into modular layers:

### 1. Mesh Core (Radio-Independent)

- Packet format (versioned)
- TTL handling & deduplication
- Routing & forwarding logic
- Reliability (ACKs, retries)

### 2. Radio Adapter Interface

Defines a standard API:

```cpp
send(packet);
onReceive(callback);
```

Supports:
- RSSI / SNR metadata (if available)
- Payload size constraints

### 3. Radio Adapters

- ESP-NOW Adapter (development)
- LoRa Adapter (deployment)

### 4. Diagnostics

- Logging
- Metrics counters
- Test hooks / simulation support

---

## 🗺️ Roadmap

### 🟢 Milestone A — Spec + Skeleton
- Packet format defined
- Node identity scheme
- Adapter interface
- Repo structure

### 🔵 Milestone B — ESP-NOW MVP
- Send/receive working
- Broadcast flooding with TTL + dedup
- Basic unicast + ACK + retry
- Serial debug tools

### 🟡 Milestone C — Routing Upgrade
Choose and implement:
- Flooding (simple)
- Distance-vector (balanced)
- Source routing (controlled)

Includes:
- Route discovery
- Loop prevention
- Performance testing

### 🟣 Milestone D — LoRa Integration
- LoRa adapter
- Airtime & payload constraints
- Fragmentation (if needed)
- Field testing (range, latency, reliability)

### 🔴 Milestone E — Hardening
- Power optimization
- Security (keys, auth, encryption)
- OTA updates
- Documentation & examples

---

## 🧪 Testing Strategy

### Unit Tests
- Packet encode/decode
- Deduplication cache
- TTL behavior

### Bench Tests
- 2-node communication
- 3-node relay
- 5–10 node stress tests

### Field Tests
- Range testing
- Obstruction scenarios
- Battery life evaluation

---

## 📦 Deliverables

- 📄 Architecture documentation  
- 💻 Reference implementation (core + adapters)  
- 🧪 Example applications:
  - Ping tool
  - Broadcast chat
  - Sensor uplink
- 📊 Test reports & known limitations  

---

## ⚠️ Risks & Mitigations

| Risk | Mitigation |
|------|-----------|
| LoRa airtime limits | Minimize chatter, compress headers |
| Flooding storms | Deduplication + jitter + rate limiting |
| Key management complexity | Start with authenticity, expand later |

---

## 🚀 Getting Started

```bash
git clone https://github.com/yourusername/your-repo.git
cd your-repo
```

### Suggested Structure

```
/core        # Mesh core logic
/adapters    # ESP-NOW, LoRa implementations
/examples    # Demo apps
/docs        # Architecture + specs
```

---

## 🤝 Contributing

Contributions are welcome. Focus areas:

- Routing algorithms
- Power optimization
- Security enhancements
- Additional radio adapters

---

## 📜 License

[Add your license here]
=======
# ESP-NOW Mesh-Like ESP32 Nodes

This project gives every ESP32 the same firmware so each board can:

- discover nearby nodes using ESP-NOW broadcast
- store peers dynamically at runtime
- send unicast or broadcast packets
- react to message types instead of fixed device roles

## Message model

Each packet uses a shared `MeshMessage` struct with:

- `type`: the behavior selector
- `senderMac`: sender identity
- `targetMac`: specific recipient or broadcast
- `nodeName`: friendly node label derived from MAC
- `text`: optional payload for text or status messages

Current message types:

- `DISCOVERY`: announces presence to the network
- `DISCOVERY_ACK`: confirms a node heard discovery
- `PING`: triggers a unicast `STATUS` reply
- `TEXT`: arbitrary application text payload
- `STATUS`: periodic liveness or response packet

## How it works

1. Every node starts in `WIFI_STA` mode on the same ESP-NOW channel.
2. Each node adds the ESP-NOW broadcast address as a peer.
3. Nodes periodically broadcast `DISCOVERY` packets.
4. When a node hears a valid packet, it stores the sender in a local peer table and adds that sender as an ESP-NOW peer.
5. Behavior is selected by `message.type`, so the same firmware can evolve into different logical behaviors without changing device roles.

## Serial commands

Open the serial monitor at `115200` baud and use:

- `help`
- `peers`
- `discover`
- `ping AA:BB:CC:DD:EE:FF`
- `send AA:BB:CC:DD:EE:FF hello there`
- `broadcast hello everyone`

## Build and flash

This project is set up for PlatformIO.

```bash
python3 -m platformio run
python3 -m platformio run -t upload
python3 -m platformio device monitor
```

## Notes

- All nodes must use the same ESP-NOW channel. This project defaults to channel `6`.
- The peer table is runtime-managed and prunes entries that have not been seen recently.
- The current code is a mesh-like discovery and messaging layer, not a full routed multi-hop mesh.
- If you want true relaying next, the clean extension is to add TTL, message IDs, and rebroadcast rules for selected packet types.
>>>>>>> master
