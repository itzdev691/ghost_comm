#include "Network/mesh.h"

#include <WiFi.h>
#include <esp_arduino_version.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include <cstring>

#include "Display/oled_ui.h"
#include "Identity/node_identity.h"
#include "Mesh/mesh_packet.h"
#include "Network/node_state.h"
#include "Network/peer_registry.h"
#include "Status/onboard_led.h"

#include "Config/app_config.h"

uint8_t selfMac[6];
char selfNodeId[NODE_ID_LEN] = "";

const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint32_t lastDiscoveryMs = 0;

struct PendingAck {
  bool active = false;
  uint8_t mac[6] = {0};
  uint32_t sendAtMs = 0;
};

static PendingAck g_pendingAcks[MAX_PEERS];
static uint32_t g_nextMessageId = 1;

struct SeenMessage {
  bool active = false;
  uint8_t originMac[6] = {0};
  uint32_t messageId = 0;

};

struct PendingForward {
  bool active = false;
  MeshMessage message = {};
  uint32_t sendAtMs = 0;
};

static SeenMessage g_seenMessages[SEEN_MESSAGE_CACHE_SIZE];
static PendingForward g_pendingForwards[MAX_PEERS];

static void initNodeId() {
  const char* configuredNodeName = getNodeName();
  if (configuredNodeName != nullptr && configuredNodeName[0] != '\0') {
    std::snprintf(selfNodeId, sizeof(selfNodeId), "%s", configuredNodeName);
    return;
  }

  std::snprintf(
      selfNodeId,
      sizeof(selfNodeId),
      "node-%02X%02X%02X",
      selfMac[3],
      selfMac[4],
      selfMac[5]);
}

String macToString(const uint8_t* macAddr) {
  char buffer[18];
  snprintf(
      buffer,
      sizeof(buffer),
      "%02X:%02X:%02X:%02X:%02X:%02X",
      macAddr[0],
      macAddr[1],
      macAddr[2],
      macAddr[3],
      macAddr[4],
      macAddr[5]);
  return String(buffer);
}

void configureWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  delay(100);
  WiFi.macAddress(selfMac);

  bool allZeros = true;
  for (int i = 0; i < 6; i++) {
    if (selfMac[i] != 0) {
      allZeros = false;
      break;
    }
  }
  if (allZeros) {
    Serial.println("WARNING: Got all-zeros MAC address, retrying...");
    delay(500);
    WiFi.macAddress(selfMac);
  }

  initNodeId();
}

static void logSendResult(const uint8_t* macAddr, esp_now_send_status_t status) {
  Serial.printf(
      "Send to %s -> %s\n",
      macToString(macAddr).c_str(),
      status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");
}

static PendingAck* findPendingAck(const uint8_t* macAddr) {
  for (size_t i = 0; i < MAX_PEERS; i++) {
    if (g_pendingAcks[i].active && std::memcmp(g_pendingAcks[i].mac, macAddr, sizeof(g_pendingAcks[i].mac)) == 0) {
      return &g_pendingAcks[i];
    }
  }

  return nullptr;
}

static PendingAck* reservePendingAckSlot() {
  for (size_t i = 0; i < MAX_PEERS; i++) {
    if (!g_pendingAcks[i].active) {
      return &g_pendingAcks[i];
    }
  }

  return nullptr;
}

static void scheduleDiscoveryAck(const uint8_t* targetMac) {
  if (targetMac == nullptr) {
    return;
  }

  PendingAck* pendingAck = findPendingAck(targetMac);
  if (pendingAck == nullptr) {
    pendingAck = reservePendingAckSlot();
  }

  if (pendingAck == nullptr) {
    Serial.printf("Dropped delayed ACK for %s: queue full\n", macToString(targetMac).c_str());
    return;
  }

  pendingAck->active = true;
  std::memcpy(pendingAck->mac, targetMac, sizeof(pendingAck->mac));
  pendingAck->sendAtMs = millis() + DISCOVERY_ACK_DELAY_MS;
  Serial.printf(
      "Queued ACK to %s in %lu ms\n",
      macToString(targetMac).c_str(),
      static_cast<unsigned long>(DISCOVERY_ACK_DELAY_MS));
}

static bool sameMessageId(const MeshMessage& a, const MeshMessage& b) {
  return std::memcmp(a.originMac, b.originMac, sizeof(a.originMac)) == 0 &&
         a.messageId == b.messageId;
}

static bool hasSeenMessage(const MeshMessage& message) {
  for (size_t i = 0; i < SEEN_MESSAGE_CACHE_SIZE; i++) {
    if (!g_seenMessages[i].active) {
      continue;
    }

    if (std::memcmp(g_seenMessages[i].originMac, message.originMac, sizeof(message.originMac)) == 0 &&
        g_seenMessages[i].messageId == message.messageId) {
      return true;
    }
  }

  return false;
}

static void rememberSeenMessage(const MeshMessage& message) {
  for (size_t i = 0; i < SEEN_MESSAGE_CACHE_SIZE; i++) {
    if (!g_seenMessages[i].active) {
      g_seenMessages[i].active = true;
      std::memcpy(g_seenMessages[i].originMac, message.originMac, sizeof(message.originMac));
      g_seenMessages[i].messageId = message.messageId;
      return;
    }
  }

  static size_t overwriteIndex = 0;
  g_seenMessages[overwriteIndex].active = true;
  std::memcpy(g_seenMessages[overwriteIndex].originMac, message.originMac, sizeof(message.originMac));
  g_seenMessages[overwriteIndex].messageId = message.messageId;
  overwriteIndex = (overwriteIndex + 1) % SEEN_MESSAGE_CACHE_SIZE;
}

static bool shouldForward(MessageType type) {
  return type != MessageType::DiscoveryAck;
}

static bool sendMeshMessage(const uint8_t* targetMac, const MeshMessage& message) {
  if (targetMac == nullptr) {
    Serial.println("Send failed: target MAC is missing");
    return false;
  }

  if (std::memcmp(targetMac, BROADCAST_MAC, 6) != 0 && !ensureEspNowPeer(targetMac)) {
    return false;
  }

  esp_err_t err = esp_now_send(targetMac, reinterpret_cast<const uint8_t*>(&message), sizeof(message));
  if (err != ESP_OK) {
    Serial.printf("Send failed: %d\n", err);
    setDisplayStatus("Send failed");
    setDisplayEvent(message.text);
    renderDisplay();
    return false;
  }

  return true;
}

static void queueForward(const MeshMessage& incoming) {
  if (incoming.ttl == 0) {
    return;
  }

  for (size_t i = 0; i < MAX_PEERS; i++) {
    if (g_pendingForwards[i].active && sameMessageId(g_pendingForwards[i].message, incoming)) {
      return;
    }
  }

  for (size_t i = 0; i < MAX_PEERS; i++) {
    if (g_pendingForwards[i].active) {
      continue;
    }

    g_pendingForwards[i].active = true;
    g_pendingForwards[i].message = incoming;
    g_pendingForwards[i].message.ttl--;
    std::memcpy(g_pendingForwards[i].message.senderMac, selfMac, sizeof(selfMac));
    std::snprintf(g_pendingForwards[i].message.senderId, sizeof(g_pendingForwards[i].message.senderId), "%s", selfNodeId);
    g_pendingForwards[i].sendAtMs = millis() + random(FORWARD_DELAY_MIN_MS, FORWARD_DELAY_MAX_MS + 1);
    return;
  }
  
  Serial.println("Dropped forward: queue full");
}

bool ensureEspNowPeer(const uint8_t* macAddr) {
  if (esp_now_is_peer_exist(macAddr)) {
    return true;
  }

  esp_now_peer_info_t peerInfo = {};
  std::memcpy(peerInfo.peer_addr, macAddr, 6);
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.encrypt = false;

  esp_err_t err = esp_now_add_peer(&peerInfo);
  if (err != ESP_OK && err != ESP_ERR_ESPNOW_EXIST) {
    Serial.printf("Failed to add peer %s: %d\n", macToString(macAddr).c_str(), err);
    return false;
  }

  return true;
}

void removeEspNowPeer(const uint8_t* macAddr) {
  if (macAddr == nullptr || std::memcmp(macAddr, BROADCAST_MAC, 6) == 0) {
    return;
  }

  if (!esp_now_is_peer_exist(macAddr)) {
    return;
  }

  esp_err_t err = esp_now_del_peer(macAddr);
  if (err != ESP_OK) {
    Serial.printf("Failed to remove peer %s: %d\n", macToString(macAddr).c_str(), err);
  }
}

static void processIncomingPacket(const uint8_t* macAddr, const uint8_t* incomingData, int len) {
  if (macAddr == nullptr) {
    Serial.println("Dropped packet without source MAC");
    return;
  }

  if (incomingData == nullptr || len != sizeof(MeshMessage)) {
    Serial.printf("Dropped packet with unexpected size: %d\n", len);
    return;
  }

  MeshMessage message = {};
  std::memcpy(&message, incomingData, sizeof(message));

  if (!isValidMeshMessage(message)) {
    Serial.println("Dropped packet with invalid protocol");
    return;
  }

  if (std::memcmp(macAddr, selfMac, 6) == 0) {
    return;
  }

  if (hasSeenMessage(message)) {
    Serial.printf(
        "Dropped duplicate message %lu from %s\n",
        static_cast<unsigned long>(message.messageId),
        macToString(macAddr).c_str());
    return;
  }

  rememberSeenMessage(message);

  noteReceiveActivity();
  upsertPeer(macAddr, message.senderId);

  MessageType type = static_cast<MessageType>(message.type);
  switch (type) {
    case MessageType::Discovery:
      Serial.printf(
          "DISCOVERY from %s (%s) -> %s\n",
          message.senderId[0] != '\0' ? message.senderId : "unknown",
          macToString(macAddr).c_str(),
          message.text);
      setDisplayStatus("Discovery rx");
      setDisplayEventFromPeer("DISC", macAddr);
      renderDisplay();
      scheduleDiscoveryAck(macAddr);
      printPeers();
      break;

    case MessageType::DiscoveryAck:
      Serial.printf(
          "DISCOVERY_ACK from %s (%s) -> %s\n",
          message.senderId[0] != '\0' ? message.senderId : "unknown",
          macToString(macAddr).c_str(),
          message.text);
      setDisplayStatus("Discovery ack");
      setDisplayEventFromPeer("ACK", macAddr);
      renderDisplay();
      printPeers();
      break;

    case MessageType::Status:
      Serial.printf(
          "STATUS from %s (%s) -> %s\n",
          message.senderId[0] != '\0' ? message.senderId : "unknown",
          macToString(macAddr).c_str(),
          message.text);
      setDisplayStatus(message.text);
      setDisplayEventFromPeer("STAT", macAddr);
      renderDisplay();
      break;

    default:
      Serial.printf(
          "Ignored unknown message type %u from %s\n",
          message.type,
          macToString(macAddr).c_str());
      break;
  }

  if (message.ttl > 0 && shouldForward(type)) {
    queueForward(message);
  }
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
static void onDataSent(const esp_now_send_info_t* txInfo, esp_now_send_status_t status) {
  if (txInfo == nullptr || txInfo->des_addr == nullptr) {
    Serial.printf("Send completed -> %s\n", status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");
    return;
  }

  logSendResult(txInfo->des_addr, status);
}
#else
static void onDataSent(const uint8_t* macAddr, esp_now_send_status_t status) {
  logSendResult(macAddr, status);
}
#endif

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
static void onDataRecv(const esp_now_recv_info_t* recvInfo, const uint8_t* incomingData, int len) {
  if (recvInfo == nullptr || recvInfo->src_addr == nullptr) {
    Serial.println("Dropped packet without receive metadata");
    return;
  }

  processIncomingPacket(recvInfo->src_addr, incomingData, len);
}
#else
static void onDataRecv(const uint8_t* macAddr, const uint8_t* incomingData, int len) {
  processIncomingPacket(macAddr, incomingData, len);
}
#endif

void initEspNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    delay(3000);
    ESP.restart();
  }

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  esp_now_peer_info_t peerInfo = {};
  std::memcpy(peerInfo.peer_addr, BROADCAST_MAC, 6);
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.encrypt = false;

  esp_err_t err = esp_now_add_peer(&peerInfo);
  if (err != ESP_OK && err != ESP_ERR_ESPNOW_EXIST) {
    Serial.printf("Failed to add broadcast peer: %d\n", err);
  } else {
    Serial.println("ESP-NOW ready");
    setDisplayStatus("ESP-NOW ready");
    setDisplayEvent("Broadcast peer");
    renderDisplay();
  }
}

void loopMesh() {
  uint32_t now = millis();

  for (size_t i = 0; i < MAX_PEERS; i++) {
    PendingAck& pendingAck = g_pendingAcks[i];
    if (!pendingAck.active) {
      continue;
    }

    if (static_cast<int32_t>(now - pendingAck.sendAtMs) < 0) {
      continue;
    }

    sendDiscoveryAck(pendingAck.mac);
    pendingAck.active = false;
    std::memset(pendingAck.mac, 0, sizeof(pendingAck.mac));
    pendingAck.sendAtMs = 0;
  }

  for (size_t i = 0; i < MAX_PEERS; i++) {
    PendingForward& pendingForward = g_pendingForwards[i];
    if (!pendingForward.active) {
      continue;
    }

    if (static_cast<int32_t>(now - pendingForward.sendAtMs) < 0) {
      continue;
    }

    sendMeshMessage(BROADCAST_MAC, pendingForward.message);
    pendingForward.active = false;
    pendingForward.message = {};
    pendingForward.sendAtMs = 0;
  }
}


bool sendMessage(const uint8_t* targetMac, MessageType type, const char* text) {
  MeshMessage message = {};
  populateMeshMessage(
      message,
      type,
      selfMac,
      selfMac,
      g_nextMessageId++,
      DEFAULT_FORWARD_TTL,
      selfNodeId,
      text);

  rememberSeenMessage(message);
  return sendMeshMessage(targetMac, message);
}


void sendDiscovery() {
  lastDiscoveryMs = millis();
  setDisplayStatus("Broadcast discovery");
  setDisplayEvent("hello from node");
  renderDisplay();
  sendMessage(BROADCAST_MAC, MessageType::Discovery, "hello from node");
}

void sendDiscoveryAck(const uint8_t* targetMac) {
  setDisplayStatus("Sending ACK");
  setDisplayEventFromPeer("ACK: ", targetMac);
  renderDisplay();
  sendMessage(targetMac, MessageType::DiscoveryAck, "ack");
}
