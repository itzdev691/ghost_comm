#pragma once

#include <stddef.h>
#include <stdint.h>

#include "Network/mesh.h"

// Shared packet constants. All nodes must agree on these values to understand each other.
static constexpr uint32_t MAGIC = 0x47484F53;  // "GHOS"
static constexpr uint16_t PROTOCOL_VERSION = 3;
static constexpr size_t NODE_ID_LEN = 16;
static constexpr size_t MESSAGE_TEXT_LEN = 32;

struct MeshMessage {
  uint32_t magic;
  uint16_t version;
  uint8_t type;
  uint8_t ttl;
  uint8_t senderMac[6];
  uint8_t originMac[6];
  uint32_t messageId;
  char senderId[NODE_ID_LEN];
  char text[MESSAGE_TEXT_LEN];
} __attribute__((packed));

void populateMeshMessage(
    MeshMessage& message,
    MessageType type,
    const uint8_t* senderMac,
    const uint8_t* originMac,
    uint32_t messageId,
    uint8_t ttl,
    const char* senderId,
    const char* text);

bool isValidMeshMessage(const MeshMessage& message);

