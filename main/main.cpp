#include <Arduino.h>

#include "Config/app_config.h"
#include "Display/oled_ui.h"
#include "Input/status_button.h"
#include "Network/mesh.h"
#include "Network/node_state.h"
#include "Network/peer_registry.h"
#include "Status/onboard_led.h"
#include "CrashLog/CrashLogger.h"
#include <esp_task_wdt.h>

CrashLogger crashLogger;

static void onPeerNewUi(const uint8_t* macAddr) {
  setDisplayStatus("Peer learned");
  setDisplayEventFromPeer("NEW", macAddr);
  renderDisplay();
}

static void onPeerLostUi(const uint8_t* macAddr) {
  setDisplayStatus("Peer timed out");
  setDisplayEventFromPeer("LOST", macAddr);
  renderDisplay();
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Longer delay for C5 USB CDC
  
  Serial.println("\n\n=== ZypheraMesh Boot ===");
  
  // Initialize crash logger FIRST - before watchdog
  Serial.println("Initializing crash logger...");
  crashLogger.begin();
  
  
  Serial.println("System ready");
  pinMode(STATUS_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(BAD_STATUS_PIN, INPUT_PULLDOWN);

  Serial.println("Configuring WiFi...");
  
  configureWiFi();
  
  Serial.println("Initializing onboard LED...");
  initOnboardLed();

  PeerUiCallbacks peerUi{};
  peerUi.onNewPeer = onPeerNewUi;
  peerUi.onPeerLost = onPeerLostUi;
  peerRegistrySetUiCallbacks(peerUi);

  Serial.println("Initializing display...");

  
  bool displayOk = initDisplay();
  
  if (!displayOk) {
    Serial.println("WARNING: Display initialization failed, continuing anyway");
  }

  // Show any crash logs from the previous boot on the OLED before continuing
  if (displayOk) {
    showCrashLogOnDisplay();
  }

  Serial.println("Initializing ESP-NOW...");
  
  initEspNow();

  if (displayOk) {
    setDisplayStatus("ESP-NOW ready");
    delay(1000);
    setDisplayEvent("Waiting...");
    renderDisplay();
  }


  Serial.printf("MAC: %s\n", macToString(selfMac).c_str());
  Serial.printf("Node ID: %s\n", selfNodeId);
  Serial.printf("Good status pin: GPIO %u\n", STATUS_BUTTON_PIN);
  Serial.printf("Bad status pin: GPIO %u\n", BAD_STATUS_PIN);
  Serial.printf("Onboard LED pin: GPIO %u\n", ONBOARD_LED_PIN);
  Serial.printf("OLED SDA: GPIO %u, SCL: GPIO %u\n", OLED_SDA_PIN, OLED_SCL_PIN);
  Serial.printf("Press GPIO %u for Good, GPIO %u for Bad\n", STATUS_BUTTON_PIN, BAD_STATUS_PIN);
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());

  esp_task_wdt_reset();
  sendDiscovery();
  
  Serial.println("=== Boot Complete ===\n");
}

void loop() {
  
  loopDisplay();
  loopOnboardLed();
  loopMesh();

  handleStatusInputs();

  if (millis() - lastDiscoveryMs >= DISCOVERY_INTERVAL_MS) {
    lastDiscoveryMs = millis();
    sendDiscovery();
  }

  prunePeers();
  delay(20);
}