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