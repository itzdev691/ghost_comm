#pragma once

#include <stddef.h>
#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

#include "Mesh/mesh_packet.h"

struct PeerInfo {
  bool active = false;
  uint8_t mac[6] = {0};
  char nodeId[NODE_ID_LEN] = "";
  uint32_t lastSeenMs = 0;
};

struct PeerUiCallbacks {
  void (*onNewPeer)(const uint8_t* macAddr) = nullptr;
  void (*onPeerLost)(const uint8_t* macAddr) = nullptr;
};

extern portMUX_TYPE g_peerMux;

void peerRegistrySetUiCallbacks(PeerUiCallbacks callbacks);

PeerInfo* findPeer(const uint8_t* macAddr);
PeerInfo* reservePeerSlot();
size_t activePeerCount();
void upsertPeer(const uint8_t* macAddr, const char* senderId);
void printPeers();
void prunePeers();
