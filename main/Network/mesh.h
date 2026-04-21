#pragma once

#include <stdint.h>

#include <Arduino.h>

enum class MessageType : uint8_t {
  Discovery = 1,
  DiscoveryAck = 2,
  Status = 3,
};

extern const uint8_t BROADCAST_MAC[6];

void configureWiFi();
void initEspNow();

bool ensureEspNowPeer(const uint8_t* macAddr);
void removeEspNowPeer(const uint8_t* macAddr);

bool sendMessage(const uint8_t* targetMac, MessageType type, const char* text);
void sendDiscovery();
void sendDiscoveryAck(const uint8_t* targetMac);

extern uint32_t lastDiscoveryMs;

String macToString(const uint8_t* macAddr);
