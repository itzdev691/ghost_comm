#pragma once

#include <stdint.h>

enum class MessageType : uint8_t {
  Discovery = 1,
  DiscoveryAck = 2,
  Status = 3,
};

extern const uint8_t BROADCAST_MAC[6];

bool sendMessage(const uint8_t* targetMac, MessageType type, const char* text);
