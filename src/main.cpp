#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h>
#include <esp_arduino_version.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Wire.h>

#include <cstring>

#include "Identity/node_identity.h"
#include "Network/mesh.h"
#include "Mesh/mesh_packet.h"
#include "Status/status_ping.h"

// Shared runtime settings. Packet layout and protocol constants live in mesh_packet.h.
constexpr uint8_t ESPNOW_CHANNEL = 6;
constexpr uint32_t DISCOVERY_INTERVAL_MS = 5000;
// GPIO pins - defined via build_flags per board
#ifndef STATUS_BUTTON_PIN
#define STATUS_BUTTON_PIN 12  // Default fallback
#endif
#ifndef OLED_SDA_PIN 
#define OLED_SDA_PIN 13  // Default fallback
#endif
#ifndef OLED_SCL_PIN 
#define OLED_SCL_PIN 27  // Default fallback
#endif
constexpr uint32_t BUTTON_DEBOUNCE_MS = 50;
constexpr uint8_t OLED_I2C_ADDRESS_PRIMARY = 0x3C;
constexpr uint8_t OLED_I2C_ADDRESS_SECONDARY = 0x3D;
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr int8_t OLED_RESET_PIN = -1;
constexpr size_t MAX_PEERS = 10;
constexpr uint32_t PEER_TIMEOUT_MS = 30000;

// Runtime record for a learned neighbor.
struct PeerInfo {
  bool active = false;
  uint8_t mac[6] = {0};
  char nodeId[NODE_ID_LEN] = "";
  uint32_t lastSeenMs = 0;
};

// Global node state used by setup, loop, and the ESP-NOW callbacks.
uint8_t selfMac[6];
char selfNodeId[NODE_ID_LEN] = "";
PeerInfo peers[MAX_PEERS];
const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
Adafruit_SH1106G oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET_PIN);
uint32_t lastDiscoveryMs = 0;
bool oledReady = false;
uint8_t oledAddress = OLED_I2C_ADDRESS_PRIMARY;
char oledStatusLine[24] = "Booting...";
char oledEventLine[24] = "Waiting...";
bool lastButtonReading = LOW;
bool buttonState = LOW;
uint32_t lastButtonChangeMs = 0;

// Forward declarations keep the file readable without forcing a different layout.
bool initDisplay();
void initEspNow();
bool probeI2CAddress(uint8_t address);
void initNodeId();
bool ensureEspNowPeer(const uint8_t* macAddr);
bool sendMessage(const uint8_t* targetMac, MessageType type, const char* text);
void sendDiscovery();
void sendDiscoveryAck(const uint8_t* targetMac);
void handleStatusButton();
size_t activePeerCount();
void removeEspNowPeer(const uint8_t* macAddr);
void renderDisplay();
void setDisplayEvent(const char* message);
void setDisplayEventFromMac(const char* label, const uint8_t* macAddr);
void setDisplayEventFromPeer(const char* label, const uint8_t* macAddr);
void setDisplayStatus(const char* message);
PeerInfo* findPeer(const uint8_t* macAddr);
PeerInfo* reservePeerSlot();
void upsertPeer(const uint8_t* macAddr, const char* senderId);
void printPeers();
void prunePeers();
void processIncomingPacket(const uint8_t* macAddr, const uint8_t* incomingData, int len);
void logSendResult(const uint8_t* macAddr, esp_now_send_status_t status);

// Arduino-ESP32 2.x and 3.x use different callback signatures, so support both.
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void onDataSent(const esp_now_send_info_t* txInfo, esp_now_send_status_t status);
void onDataRecv(const esp_now_recv_info_t* recvInfo, const uint8_t* incomingData, int len);
#else
void onDataSent(const uint8_t* macAddr, esp_now_send_status_t status);
void onDataRecv(const uint8_t* macAddr, const uint8_t* incomingData, int len);
#endif

// Format a MAC address once so logs stay readable everywhere else in the file.
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

// Put the radio into station mode and force the channel ESP-NOW will use.
void configureWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  // Wait a moment for WiFi to be ready before getting MAC address
  delay(100);
  WiFi.macAddress(selfMac);
  
  // Debug: verify MAC address is not all zeros
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

void initNodeId() {
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

// Detect whether the OLED responds on a common SSD1306 I2C address.
bool probeI2CAddress(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

bool initDisplay() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

  // Force 0x3D as requested, but keep a safe fallback selector
  if (probeI2CAddress(OLED_I2C_ADDRESS_SECONDARY)) {
    oledAddress = OLED_I2C_ADDRESS_SECONDARY;
  } else if (probeI2CAddress(OLED_I2C_ADDRESS_PRIMARY)) {
    oledAddress = OLED_I2C_ADDRESS_PRIMARY;
  } else {
    Serial.println("OLED I2C not found at 0x3C or 0x3D");
    oledReady = false;
    return false;
  }

  if (!oled.begin(oledAddress, true)) {
    Serial.printf("OLED begin() failed at 0x%02X\n", oledAddress);
    oledReady = false;
    return false;
  }

  oledReady = true;
  // Test display: fill white, then black
  oled.fillScreen(SH110X_WHITE);
  oled.display();
  delay(500);
  oled.fillScreen(SH110X_BLACK);
  oled.display();
  delay(500);

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SH110X_WHITE);
  oled.setCursor(0, 0);
  oled.println("Booting..");
  oled.display();
  delay(1000);
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SH110X_WHITE);
  oled.setCursor(0, 0);
  oled.println("Booting..");
  oled.display();
  delay(1000);
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SH110X_WHITE);
  oled.setCursor(0, 0);
  oled.println("Booting...");
  oled.display();
  delay(2000);

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SH110X_WHITE);
  oled.setCursor(0, 0);
  oled.println("ESP-NOW initialized");
  oled.display();
  delay(2000);

  return true;
}




// Count active peers so the display can summarize network state at a glance.
size_t activePeerCount() {
  size_t count = 0;

  for (size_t i = 0; i < MAX_PEERS; i++) {
    if (peers[i].active) {
      count++;
    }
  }

  return count;
}

// Keep the OLED text buffers small and predictable.
void setDisplayStatus(const char* message) {
  std::snprintf(oledStatusLine, sizeof(oledStatusLine), "%s", message != nullptr ? message : "");
}

void setDisplayEvent(const char* message) {
  std::snprintf(oledEventLine, sizeof(oledEventLine), "%s", message != nullptr ? message : "");
}

void setDisplayEventFromMac(const char* label, const uint8_t* macAddr) {
  if (macAddr == nullptr) {
    setDisplayEvent(label);
    return;
  }

  std::snprintf(
      oledEventLine,
      sizeof(oledEventLine),
      "%s %02X:%02X:%02X",
      label != nullptr ? label : "MAC",
      macAddr[3],
      macAddr[4],
      macAddr[5]);
}

void setDisplayEventFromPeer(const char* label, const uint8_t* macAddr) {
  PeerInfo* peer = findPeer(macAddr);
  if (peer == nullptr || peer->nodeId[0] == '\0') {
    setDisplayEventFromMac(label, macAddr);
    return;
  }

  std::snprintf(
      oledEventLine,
      sizeof(oledEventLine),
      "%s %s",
      label != nullptr ? label : "ID",
      peer->nodeId);
}

// Render a compact status screen showing identity, peers, and the latest event.
void renderDisplay() {
  if (!oledReady) {
    return;
  }

  char selfLine[24];
  char peerLine[24];

  std::snprintf(selfLine, sizeof(selfLine), "Self: %s", selfNodeId);
  std::snprintf(peerLine, sizeof(peerLine), "Peers: %u", static_cast<unsigned>(activePeerCount()));

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SH110X_WHITE);
  oled.setCursor(0, 0);
  oled.println(F("ESP-NOW Mesh"));
  oled.println(selfLine);
  oled.println(peerLine);
  oled.println(oledStatusLine);
  oled.println(oledEventLine);
#ifdef BOARD_ESP32_DOIT
  oled.println(F("ESP32-DOIT"));
#elif defined(BOARD_ESP32_S3_ZERO)
  oled.println(F("ESP32-S3 Zero"));
#else
  oled.println(F("Unknown Board"));
#endif
  oled.display();
}

// Find an already-known peer by MAC address.
PeerInfo* findPeer(const uint8_t* macAddr) {
  for (size_t i = 0; i < MAX_PEERS; i++) {
    if (peers[i].active && std::memcmp(peers[i].mac, macAddr, 6) == 0) {
      return &peers[i];
    }
  }

  return nullptr;
}

// Grab the first empty slot in the fixed-size peer table.
PeerInfo* reservePeerSlot() {
  for (size_t i = 0; i < MAX_PEERS; i++) {
    if (!peers[i].active) {
      return &peers[i];
    }
  }

  return nullptr;
}

// ESP-NOW requires a peer to be registered before we can unicast to it.
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

// Learn a new peer or refresh the last-seen time for one we already know.
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
    setDisplayStatus("Peer learned");
    setDisplayEventFromPeer("NEW", macAddr);
    renderDisplay();
  }

  ensureEspNowPeer(macAddr);
  peer->lastSeenMs = millis();
}

// Remove a peer from ESP-NOW's internal list when we no longer want to unicast to it.
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

// Debug helper to show which peers this node currently believes are alive.
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

// Age out nodes that have gone quiet for too long.
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
    setDisplayStatus("Peer timed out");
    setDisplayEventFromPeer("LOST", peers[i].mac);
    renderDisplay();
    removeEspNowPeer(peers[i].mac);
    peers[i].active = false;
    std::memset(peers[i].mac, 0, sizeof(peers[i].mac));
    std::memset(peers[i].nodeId, 0, sizeof(peers[i].nodeId));
    peers[i].lastSeenMs = 0;
  }
}

// Arduino startup: initialize serial, configure the radio, and announce ourselves.
void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(STATUS_BUTTON_PIN, INPUT_PULLDOWN);

  configureWiFi();
  initDisplay();
  initEspNow();

  setDisplayStatus("ESP-NOW ready");
  delay(2000);
  setDisplayEvent("Waiting...");
  renderDisplay();

  Serial.println("ESP32 starting...");
#ifdef BOARD_ESP32_DOIT
  Serial.println("Board: ESP32-DOIT DevKit v1");
#elif defined(BOARD_ESP32_S3_ZERO)
  Serial.println("Board: Waveshare ESP32-S3 Zero");
#else
  Serial.println("Board: Unknown (using default pins)");
#endif
  Serial.printf("MAC: %s\n", macToString(selfMac).c_str());
  Serial.printf("Node ID: %s\n", selfNodeId);
  Serial.printf("Button pin: GPIO %u\n", STATUS_BUTTON_PIN);
  Serial.printf("OLED SDA: GPIO %u, SCL: GPIO %u\n", OLED_SDA_PIN, OLED_SCL_PIN);
  Serial.printf("Press button on GPIO %u to broadcast status: Good\n", STATUS_BUTTON_PIN);
  Serial.printf("OLED I2C wiring SDA=%u SCL=%u\n", OLED_SDA_PIN, OLED_SCL_PIN);

  sendDiscovery();
}

// Main loop: watch the button, periodically announce ourselves, and prune stale peers.
void loop() {
  handleStatusButton();

  if (millis() - lastDiscoveryMs >= DISCOVERY_INTERVAL_MS) {
    sendDiscovery();
  }

  prunePeers();
  delay(20);
}

// Centralized send logging keeps both callback variants consistent.
void logSendResult(const uint8_t* macAddr, esp_now_send_status_t status) {
  Serial.printf(
      "Send to %s -> %s\n",
      macToString(macAddr).c_str(),
      status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");
}

// Debounce the button and only send one status message per press.
void handleStatusButton() {
  bool reading = digitalRead(STATUS_BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastButtonChangeMs = millis();
    lastButtonReading = reading;
  }

  if (millis() - lastButtonChangeMs < BUTTON_DEBOUNCE_MS) {
    return;
  }

  if (reading == buttonState) {
    return;
  }

  buttonState = reading;
  if (buttonState == HIGH) {
    Serial.println("Button pressed -> broadcasting status: Good");
    setDisplayStatus("Broadcast Good");
    setDisplayEvent("Button press");
    renderDisplay();
    sendGoodStatusPing();
  }
}

// Send callback wrappers differ by core version, but both end up in the same logger.
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void onDataSent(const esp_now_send_info_t* txInfo, esp_now_send_status_t status) {
  if (txInfo == nullptr || txInfo->des_addr == nullptr) {
    Serial.printf("Send completed -> %s\n", status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");
    return;
  }

  logSendResult(txInfo->des_addr, status);
}
#else
void onDataSent(const uint8_t* macAddr, esp_now_send_status_t status) {
  logSendResult(macAddr, status);
}
#endif

// Validate every packet once, learn the sender, then dispatch by message type.
void processIncomingPacket(const uint8_t* macAddr, const uint8_t* incomingData, int len) {
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
      sendDiscoveryAck(macAddr);
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
      Serial.printf("Ignored unknown message type %u from %s\n", message.type, macToString(macAddr).c_str());
      break;
  }
}

// Receive callback wrappers differ by core version, but both forward into the same parser.
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void onDataRecv(const esp_now_recv_info_t* recvInfo, const uint8_t* incomingData, int len) {
  if (recvInfo == nullptr || recvInfo->src_addr == nullptr) {
    Serial.println("Dropped packet without receive metadata");
    return;
  }

  processIncomingPacket(recvInfo->src_addr, incomingData, len);
}
#else
void onDataRecv(const uint8_t* macAddr, const uint8_t* incomingData, int len) {
  processIncomingPacket(macAddr, incomingData, len);
}
#endif

// Start ESP-NOW, register callbacks, and add the broadcast peer used for discovery.
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

// Build and send one packet. Broadcast and unicast both flow through this helper.
bool sendMessage(const uint8_t* targetMac, MessageType type, const char* text) {
  if (targetMac == nullptr) {
    Serial.println("Send failed: target MAC is missing");
    return false;
  }

  if (std::memcmp(targetMac, BROADCAST_MAC, 6) != 0 && !ensureEspNowPeer(targetMac)) {
    return false;
  }

  MeshMessage message = {};
  populateMeshMessage(message, type, selfMac, selfNodeId, text);
 
  esp_err_t err = esp_now_send(targetMac, reinterpret_cast<uint8_t*>(&message), sizeof(message));
  if (err != ESP_OK) {
    Serial.printf("Send failed: %d\n", err);
    setDisplayStatus("Send failed");
    setDisplayEvent(text);
    renderDisplay();
    return false;
  }

  return true;
}

// Periodic broadcast used to let nearby nodes discover us.
void sendDiscovery() {
  lastDiscoveryMs = millis();
  setDisplayStatus("Broadcast discovery");
  setDisplayEvent("hello from node");
  renderDisplay();
  sendMessage(BROADCAST_MAC, MessageType::Discovery, "hello from node");
}

// Direct reply that confirms another node heard our discovery broadcast.
void sendDiscoveryAck(const uint8_t* targetMac) {
  setDisplayStatus("Sending ACK");
  setDisplayEventFromPeer("ACK: ", targetMac);
  renderDisplay();
  sendMessage(targetMac, MessageType::DiscoveryAck, "ack");
}
