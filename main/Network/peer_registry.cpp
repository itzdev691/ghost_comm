#include "Network/peer_registry.h"

#include <Arduino.h>

#include <cstring>

#include "Config/app_config.h"
#include "Network/mesh.h"
#include "Network/node_state.h"

PeerInfo peers[MAX_PEERS];

portMUX_TYPE g_peerMux = portMUX_INITIALIZER_UNLOCKED;

static PeerUiCallbacks g_uiCallbacks;

void peerRegistrySetUiCallbacks(PeerUiCallbacks callbacks) {
  g_uiCallbacks = callbacks;
}

PeerInfo* findPeer(const uint8_t* macAddr) {
  taskENTER_CRITICAL(&g_peerMux);
  
  for (size_t i = 0; i < MAX_PEERS; i++) {
    if (peers[i].active && std::memcmp(peers[i].mac, macAddr, 6) == 0) {
      taskEXIT_CRITICAL(&g_peerMux);
      return &peers[i];
    }
  }

  taskEXIT_CRITICAL(&g_peerMux);
  return nullptr;
}

PeerInfo* reservePeerSlot() {
  taskENTER_CRITICAL(&g_peerMux);
  
  for (size_t i = 0; i < MAX_PEERS; i++) {
    if (!peers[i].active) {
      taskEXIT_CRITICAL(&g_peerMux);
      return &peers[i];
    }
  }

  taskEXIT_CRITICAL(&g_peerMux);
  return nullptr;
}

size_t activePeerCount() {
  taskENTER_CRITICAL(&g_peerMux);
  
  size_t count = 0;

  for (size_t i = 0; i < MAX_PEERS; i++) {
    if (peers[i].active) {
      count++;
    }
  }

  taskEXIT_CRITICAL(&g_peerMux);
  return count;
}

void upsertPeer(const uint8_t* macAddr, const char* senderId) {
  if (std::memcmp(macAddr, selfMac, 6) == 0) {
    return;
  }

  taskENTER_CRITICAL(&g_peerMux);
  
  PeerInfo* peer = nullptr;
  for (size_t i = 0; i < MAX_PEERS; i++) {
    if (peers[i].active && std::memcmp(peers[i].mac, macAddr, 6) == 0) {
      peer = &peers[i];
      break;
    }
  }
  
  if (peer == nullptr) {
    for (size_t i = 0; i < MAX_PEERS; i++) {
      if (!peers[i].active) {
        peer = &peers[i];
        break;
      }
    }
    
    if (peer == nullptr) {
      taskEXIT_CRITICAL(&g_peerMux);
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

  bool isNewPeer = (peer->lastSeenMs == 0);
  peer->lastSeenMs = millis();
  
  // Copy MAC and nodeId for use outside critical section
  uint8_t macCopy[6];
  char nodeIdCopy[NODE_ID_LEN];
  std::memcpy(macCopy, peer->mac, 6);
  std::strncpy(nodeIdCopy, peer->nodeId, sizeof(nodeIdCopy) - 1);
  nodeIdCopy[sizeof(nodeIdCopy) - 1] = '\0';
  
  taskEXIT_CRITICAL(&g_peerMux);

  // Call slow operations outside the critical section
  if (isNewPeer) {
    Serial.printf(
        "Learned new peer: %s (%s)\n",
        nodeIdCopy[0] != '\0' ? nodeIdCopy : "unknown",
        macToString(macCopy).c_str());
    if (g_uiCallbacks.onNewPeer != nullptr) {
      g_uiCallbacks.onNewPeer(macCopy);
    }
  }

  ensureEspNowPeer(macCopy);
}

void printPeers() {
  Serial.println("Known peers:");

  taskENTER_CRITICAL(&g_peerMux);
  
  bool found = false;
  for (size_t i = 0; i < MAX_PEERS; i++) {
    if (!peers[i].active) {
      continue;
    }

    found = true;
    uint32_t lastSeenAgo = millis() - peers[i].lastSeenMs;
    
    // Copy data for printing outside critical section
    char nodeIdCopy[NODE_ID_LEN];
    uint8_t macCopy[6];
    std::strncpy(nodeIdCopy, peers[i].nodeId, sizeof(nodeIdCopy) - 1);
    nodeIdCopy[sizeof(nodeIdCopy) - 1] = '\0';
    std::memcpy(macCopy, peers[i].mac, 6);
    
    taskEXIT_CRITICAL(&g_peerMux);
    
    Serial.printf(
        "  %s  %s  last seen %lu ms ago\n",
        nodeIdCopy[0] != '\0' ? nodeIdCopy : "(unknown)",
        macToString(macCopy).c_str(),
        static_cast<unsigned long>(lastSeenAgo));
    
    taskENTER_CRITICAL(&g_peerMux);
  }

  taskEXIT_CRITICAL(&g_peerMux);
  
  if (!found) {
    Serial.println("  (none)");
  }
}

void prunePeers() {
  uint32_t now = millis();

  // Collect peers to prune inside critical section
  struct PeerToRemove {
    uint8_t mac[6];
    char nodeId[NODE_ID_LEN];
  };
  PeerToRemove toRemove[MAX_PEERS];
  size_t removeCount = 0;

  taskENTER_CRITICAL(&g_peerMux);
  
  for (size_t i = 0; i < MAX_PEERS; i++) {
    if (!peers[i].active) {
      continue;
    }

    if (now - peers[i].lastSeenMs <= PEER_TIMEOUT_MS) {
      continue;
    }

    // Collect peer info for removal
    std::memcpy(toRemove[removeCount].mac, peers[i].mac, 6);
    std::strncpy(toRemove[removeCount].nodeId, peers[i].nodeId, sizeof(toRemove[removeCount].nodeId) - 1);
    toRemove[removeCount].nodeId[sizeof(toRemove[removeCount].nodeId) - 1] = '\0';
    removeCount++;

    // Clear the peer entry
    peers[i].active = false;
    std::memset(peers[i].mac, 0, sizeof(peers[i].mac));
    std::memset(peers[i].nodeId, 0, sizeof(peers[i].nodeId));
    peers[i].lastSeenMs = 0;
  }

  taskEXIT_CRITICAL(&g_peerMux);

  // Call slow operations outside the critical section
  for (size_t i = 0; i < removeCount; i++) {
    Serial.printf(
        "Removing stale peer: %s (%s)\n",
        toRemove[i].nodeId[0] != '\0' ? toRemove[i].nodeId : "unknown",
        macToString(toRemove[i].mac).c_str());
    if (g_uiCallbacks.onPeerLost != nullptr) {
      g_uiCallbacks.onPeerLost(toRemove[i].mac);
    }
    removeEspNowPeer(toRemove[i].mac);
  }
}
