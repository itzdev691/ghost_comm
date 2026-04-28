# 🌐 ZypheraMesh

**ZypheraMesh** is a radio-agnostic, decentralized mesh networking stack for embedded devices — built to operate without centralized infrastructure.

It enables devices to **discover peers, exchange data, and relay messages across a distributed mesh network**, with current validation on **ESP-NOW** and future deployment targeting **LoRa**.

---

## 🧭 Project Goals

Build a **modular, portable mesh networking stack** that:

- Supports multiple radio technologies
- Operates with **no central server or internet dependency**
- Enables **reliable multi-hop communication**
- Can be adapted to different embedded hardware platforms

---

## 🎯 Target Environments

| Environment | Radio | Purpose |
|------------|------|---------|
| Development | ESP-NOW | Fast iteration, low-latency local testing |
| Deployment | LoRa | Long-range, real-world mesh networking |

---

## 🚫 Current Non-Goals

The following are intentionally out of scope for now:

- Mobile applications
- Cloud dashboards
- Paid backend infrastructure
- Consumer-facing UI layers

---

## ✅ Features

## Core Networking Features

- 🔍 Automatic node discovery & neighbor awareness  
- 🆔 Unique node addressing (ID + optional label)  
- 📡 Message support:
  - Broadcast
  - Unicast
  - Acknowledgements (ACKs)

- 🔁 Relay / forwarding system:
  - TTL (time-to-live)
  - Duplicate suppression

- 🧭 Routing strategies (modular and evolving)
- 💾 Optional store-and-forward messaging

## System Design Goals

- 🔌 Radio-agnostic architecture
- 🔋 Power-efficient operation (sleep / duty-cycle ready)
- 📶 Reliable packet delivery over distance
- 📈 Scalable network growth and hop count
- 🔐 Security baseline (authenticity + optional encryption)

---

## 🏗️ Architecture

The stack is organized into modular layers.

## 1. Mesh Core (Radio Independent)

Responsible for:

- Packet format and versioning
- TTL handling
- Duplicate detection
- Routing and forwarding
- ACK / retry reliability logic

## 2. Radio Adapter Interface

Each radio backend implements a common transport API:

```cpp
send(packet);
onReceive(callback);
Firmware does **not** depend on a specific IDE. You can build with **Arduino IDE**, **arduino-cli**, **CMake** (ESP-IDF / Arduino as component), **PlatformIO**, or another toolchain that compiles the `src/` tree.

### OLED I2C pins

Default **SDA/SCL GPIO** values live in **[`src/Config/board_user_config.h`](src/Config/board_user_config.h)**. Edit that file for your wiring (or add an `#elif defined(YOUR_BOARD_MACRO)` branch).

Known layouts are selected via preprocessor symbols such as `BOARD_ESP32_DOIT`, `BOARD_ESP32_S3_ZERO`, and ESP32-C5 (`CONFIG_IDF_TARGET_ESP32C5` / `ARDUINO_ESP32C5_DEV`). Other boards use the generic fallback in that file (classic ESP32-style defaults — always verify against your schematic).

Optional: any build system may pass `-DOLED_SDA_PIN` / `-DOLED_SCL_PIN` to override both pins for automation or CI.
