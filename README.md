🌐 Decentralized Mesh Networking Stack

A radio-agnostic, decentralized mesh networking system for embedded devices — designed to run without any central server.

This project enables devices to discover, communicate, and relay messages across a mesh network, with initial validation on ESP-NOW and deployment targeting LoRa.

⸻

🧭 Purpose

The goal is to build a modular mesh networking stack that:
	•	Works across multiple radio technologies
	•	Requires no central infrastructure
	•	Supports reliable multi-hop communication

Target Environments

Environment	Radio	Purpose
Development	ESP-NOW	Fast iteration, short range
Deployment	LoRa	Long range, real-world usage

🚫 Non-Goals (for now)
	•	Mobile apps
	•	Cloud dashboards
	•	Paid infrastructure/services

⸻

✅ Features

Core Functionality
	•	🔍 Node discovery & neighbor awareness
	•	🆔 Unique node addressing (ID + optional label)
	•	📡 Message types:
	•	Broadcast
	•	Unicast
	•	Acknowledgements (ACKs)
	•	🔁 Relay system:
	•	TTL (time-to-live)
	•	Duplicate suppression
	•	🧭 Routing strategies (pluggable & evolving)
	•	💾 Optional store-and-forward support

Non-Functional Goals
	•	🔌 Radio-agnostic architecture
	•	🔋 Power-efficient operation (sleep/duty cycle)
	•	📶 Reliable packet delivery over distance
	•	📈 Scalable network size & diameter
	•	🔐 Security baseline (authenticity, optional encryption)

⸻

🏗️ Architecture

The system is split into modular layers:

1. Mesh Core (Radio-Independent)
	•	Packet format (versioned)
	•	TTL handling & deduplication
	•	Routing & forwarding logic
	•	Reliability (ACKs, retries)

2. Radio Adapter Interface

Defines a standard API:

send(packet);
onReceive(callback);

Supports:
	•	RSSI / SNR metadata (if available)
	•	Payload size constraints

3. Radio Adapters
	•	ESP-NOW Adapter (development)
	•	LoRa Adapter (deployment)

4. Diagnostics
	•	Logging
	•	Metrics counters
	•	Test hooks / simulation support

⸻

🗺️ Roadmap

🟢 Milestone A — Spec + Skeleton
	•	Packet format defined
	•	Node identity scheme
	•	Adapter interface
	•	Repo structure

🔵 Milestone B — ESP-NOW MVP
	•	Send/receive working
	•	Broadcast flooding with TTL + dedup
	•	Basic unicast + ACK + retry
	•	Serial debug tools

🟡 Milestone C — Routing Upgrade

Choose and implement:
	•	Flooding (simple)
	•	Distance-vector (balanced)
	•	Source routing (controlled)

Includes:
	•	Route discovery
	•	Loop prevention
	•	Performance testing

🟣 Milestone D — LoRa Integration
	•	LoRa adapter
	•	Airtime & payload constraints
	•	Fragmentation (if needed)
	•	Field testing (range, latency, reliability)

🔴 Milestone E — Hardening
	•	Power optimization
	•	Security (keys, auth, encryption)
	•	OTA updates
	•	Documentation & examples

⸻

🧪 Testing Strategy

Unit Tests
	•	Packet encode/decode
	•	Deduplication cache
	•	TTL behavior

Bench Tests
	•	2-node communication
	•	3-node relay
	•	5–10 node stress tests

Field Tests
	•	Range testing
	•	Obstruction scenarios
	•	Battery life evaluation

⸻

📦 Deliverables
	•	📄 Architecture documentation
	•	💻 Reference implementation (core + adapters)
	•	🧪 Example applications:
	•	Ping tool
	•	Broadcast chat
	•	Sensor uplink
	•	📊 Test reports & known limitations

⸻

⚠️ Risks & Mitigations

Risk	Mitigation
LoRa airtime limits	Minimize chatter, compress headers
Flooding storms	Deduplication + jitter + rate limiting
Key management complexity	Start with authenticity, expand later


⸻

🚀 Getting Started

git clone https://github.com/yourusername/your-repo.git
cd your-repo

Suggested Structure

/core        # Mesh core logic
/adapters    # ESP-NOW, LoRa implementations
/examples    # Demo apps
/docs        # Architecture + specs


⸻

🔧 Build System

This project is currently built using the Arduino framework (via PlatformIO), enabling rapid prototyping and ease of development on ESP32 devices.

🔮 Future Plans

The project is planned to be migrated to the ESP-IDF (Espressif IoT Development Framework) in the future. This transition will provide:
	•	Lower-level hardware control
	•	Improved performance and memory management
	•	Better scalability for advanced mesh networking features
	•	Native access to ESP32 capabilities and optimizations

The Arduino-based implementation serves as a flexible foundation while the system architecture matures.

⸻

🤝 Contributing

Contributions are welcome. Focus areas:
	•	Routing algorithms
	•	Power optimization
	•	Security enhancements
	•	Additional radio adapters

⸻

📜 License

[Add your license here]