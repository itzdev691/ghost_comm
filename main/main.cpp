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
  delay(500);
  // Initialize crash logger
  crashLogger.begin();
    
  // Setup watchdog
  esp_task_wdt_init(10, true);
  esp_task_wdt_add(NULL);
  
  Serial.println("System ready");
  delay(500);
  pinMode(STATUS_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(BAD_STATUS_PIN, INPUT_PULLDOWN);

  configureWiFi();
  initOnboardLed();

  PeerUiCallbacks peerUi{};
  peerUi.onNewPeer = onPeerNewUi;
  peerUi.onPeerLost = onPeerLostUi;
  peerRegistrySetUiCallbacks(peerUi);

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
  Serial.printf("Good status pin: GPIO %u\n", STATUS_BUTTON_PIN);
  Serial.printf("Bad status pin: GPIO %u\n", BAD_STATUS_PIN);
  Serial.printf("Onboard LED pin: GPIO %u\n", ONBOARD_LED_PIN);
  Serial.printf("OLED SDA: GPIO %u, SCL: GPIO %u\n", OLED_SDA_PIN, OLED_SCL_PIN);
  Serial.printf("Press GPIO %u for Good, GPIO %u for Bad\n", STATUS_BUTTON_PIN, BAD_STATUS_PIN);
  Serial.printf("OLED I2C wiring SDA=%u SCL=%u\n", OLED_SDA_PIN, OLED_SCL_PIN);

  sendDiscovery();
}

void loop() {
  esp_task_wdt_reset();
  loopDisplay();  // ← added, handles pending renderDisplay() safely
  loopOnboardLed();
  loopMesh();

  handleStatusInputs();

  if (millis() - lastDiscoveryMs >= DISCOVERY_INTERVAL_MS) {
    lastDiscoveryMs = millis();  // ← was missing, prevents continuous firing
    sendDiscovery();
  }

  prunePeers();
  delay(20);
}
