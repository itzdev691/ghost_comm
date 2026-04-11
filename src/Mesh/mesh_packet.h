#pragma once

#include <stddef.h>
#include <stdint.h>

#include "Network/mesh.h"

// Shared packet constants. All nodes must agree on these values to understand each other.
inline constexpr uint32_t MAGIC = 0x47484F53;  // "GHOS"
inline constexpr uint16_t PROTOCOL_VERSION = 2;
inline constexpr size_t NODE_ID_LEN = 16;
inline constexpr size_t MESSAGE_TEXT_LEN = 32;

// Compact wire format used for every ESP-NOW packet in this project.
struct MeshMessage {
  uint32_t magic;
  uint16_t version;
  uint8_t type;
  uint8_t reserved;
  uint8_t senderMac[6];
  char senderId[NODE_ID_LEN];
  char text[MESSAGE_TEXT_LEN];
} __attribute__((packed));

void populateMeshMessage(
    MeshMessage& message,
    MessageType type,
    const uint8_t* senderMac,
    const char* senderId,
    const char* text);

bool isValidMeshMessage(const MeshMessage& message);
