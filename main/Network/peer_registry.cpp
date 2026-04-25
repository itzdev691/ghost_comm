#include "Network/peer_registry.h"

#include <Arduino.h>

#include <cstring>

#include "Config/app_config.h"
#include "Network/mesh.h"
#include "Network/node_state.h"

PeerInfo peers[MAX_PEERS];

static PeerUiCallbacks g_uiCallbacks;

void peerRegistrySetUiCallbacks(PeerUiCallbacks callbacks) {
  g_uiCallbacks = callbacks;
}

PeerInfo* findPeer(const uint8_t* macAddr) {
  for (size_t i = 0; i < MAX_PEERS; i++) {
    if (peers[i].active && std::memcmp(peers[i].mac, macAddr, 6) == 0) {
      return &peers[i];
    }
  }

  return nullptr;
}

PeerInfo* reservePeerSlot() {
  for (size_t i = 0; i < MAX_PEERS; i++) {
    if (!peers[i].active) {
      return &peers[i];
    }
  }

  return nullptr;
}

size_t activePeerCount() {
  size_t count = 0;

  for (size_t i = 0; i < MAX_PEERS; i++) {
    if (peers[i].active) {
      count++;
    }
  }

  return count;
}

void upsertPeer(const uint8_t* macAddr, const char* senderId) {
  if (std::memcmp(macAddr, selfMac, 6) == 0) {
    return;
  }

  PeerInfo* peer = findPeer(macAddr);
  if (peer == nullptr) {
    peer = reservePeerSlot();
    if (peer == nullptr) {
      Serial.println("Peer table full");
      return;
    }

    peer->active = true;
    std::memcpy(peer->mac, macAddr, 6);
  }

  if (senderId != nullptr) {
    std::strncpy(peer->nodeId, senderId, sizeof(peer->nodeId) - 1);
    peer->nodeId[sizeof(peer->nodeId) - 1] = '\0';
  }

  if (peer->lastSeenMs == 0) {
    Serial.printf(
        "Learned new peer: %s (%s)\n",
        peer->nodeId[0] != '\0' ? peer->nodeId : "unknown",
        macToString(macAddr).c_str());
    if (g_uiCallbacks.onNewPeer != nullptr) {
      g_uiCallbacks.onNewPeer(macAddr);
    }
  }

  ensureEspNowPeer(macAddr);
  peer->lastSeenMs = millis();
}

void printPeers() {
  Serial.println("Known peers:");

  bool found = false;
  for (size_t i = 0; i < MAX_PEERS; i++) {
    if (!peers[i].active) {
      continue;
    }

    found = true;
    Serial.printf(
        "  %s  %s  last seen %lu ms ago\n",
        peers[i].nodeId[0] != '\0' ? peers[i].nodeId : "(unknown)",
        macToString(peers[i].mac).c_str(),
        static_cast<unsigned long>(millis() - peers[i].lastSeenMs));
  }

  if (!found) {
    Serial.println("  (none)");
  }
}

void prunePeers() {
  uint32_t now = millis();

  for (size_t i = 0; i < MAX_PEERS; i++) {
    if (!peers[i].active) {
      continue;
    }

    if (now - peers[i].lastSeenMs <= PEER_TIMEOUT_MS) {
      continue;
    }

    Serial.printf(
        "Removing stale peer: %s (%s)\n",
        peers[i].nodeId[0] != '\0' ? peers[i].nodeId : "unknown",
        macToString(peers[i].mac).c_str());
    if (g_uiCallbacks.onPeerLost != nullptr) {
      g_uiCallbacks.onPeerLost(peers[i].mac);
    }
    removeEspNowPeer(peers[i].mac);
    peers[i].active = false;
    std::memset(peers[i].mac, 0, sizeof(peers[i].mac));
    std::memset(peers[i].nodeId, 0, sizeof(peers[i].nodeId));
    peers[i].lastSeenMs = 0;
  }
}
