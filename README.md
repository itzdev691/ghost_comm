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

---

## Building and hardware configuration

Firmware does **not** depend on a specific IDE. You can build with **Arduino IDE**, **arduino-cli**, **CMake** (ESP-IDF / Arduino as component), **PlatformIO**, or another toolchain that compiles the `src/` tree.

### OLED I2C pins

Default **SDA/SCL GPIO** values live in **[`src/Config/board_user_config.h`](src/Config/board_user_config.h)**. Edit that file for your wiring (or add an `#elif defined(YOUR_BOARD_MACRO)` branch).

Known layouts are selected via preprocessor symbols such as `BOARD_ESP32_DOIT`, `BOARD_ESP32_S3_ZERO`, and ESP32-C5 (`CONFIG_IDF_TARGET_ESP32C5` / `ARDUINO_ESP32C5_DEV`). Other boards use the generic fallback in that file (classic ESP32-style defaults — always verify against your schematic).

Optional: any build system may pass `-DOLED_SDA_PIN` / `-DOLED_SCL_PIN` to override both pins for automation or CI.