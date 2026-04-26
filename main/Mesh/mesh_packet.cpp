#include "mesh_packet.h"

#include <cstring>

void populateMeshMessage(
  MeshMessage& message,
  MessageType type,
  const uint8_t* senderMac,
  const uint8_t* originMac,
  uint32_t messageId,
  uint8_t ttl,
  const char* senderId,
  const char* text) {
message = {};
message.magic = MAGIC;
message.version = PROTOCOL_VERSION;
message.type = static_cast<uint8_t>(type);
message.ttl = ttl;
message.messageId = messageId;

if (senderMac != nullptr) {
  std::memcpy(message.senderMac, senderMac, sizeof(message.senderMac));
}

if (originMac != nullptr) {
  std::memcpy(message.originMac, originMac, sizeof(message.originMac));
}

std::strncpy(message.senderId, senderId != nullptr ? senderId : "", sizeof(message.senderId) - 1);
std::strncpy(message.text, text != nullptr ? text : "", sizeof(message.text) - 1);
}


bool isValidMeshMessage(const MeshMessage& message) {
  return message.magic == MAGIC && message.version == PROTOCOL_VERSION;
}
