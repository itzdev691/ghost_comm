#include "Display/oled_ui.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Arduino.h>
#include <Wire.h>

#include <cstring>
#include <cstdio>

#include "Config/app_config.h"
#include "Network/node_state.h"
#include "Network/peer_registry.h"

Adafruit_SH1106G g_oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET_PIN);

static bool oledReady = false;
static uint8_t oledAddress = OLED_I2C_ADDRESS_PRIMARY;
static char oledStatusLine[24] = "Booting...";
static char oledEventLine[24] = "Waiting...";

static bool probeI2CAddress(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

bool initDisplay() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  Wire.setClock(100000);
  Wire.setTimeout(500);  // ← add this, increase from default 50ms
  delay(50);

  Serial.println("Probing 0x3C...");
  bool found3C = probeI2CAddress(OLED_I2C_ADDRESS_PRIMARY);
  Serial.printf("0x3C result: %d\n", found3C);

  if (found3C) {
      oledAddress = OLED_I2C_ADDRESS_PRIMARY;
  } else {
      delay(10);
      Serial.println("Probing 0x3D...");
      bool found3D = probeI2CAddress(OLED_I2C_ADDRESS_SECONDARY);
      Serial.printf("0x3D result: %d\n", found3D);

      if (found3D) {
          oledAddress = OLED_I2C_ADDRESS_SECONDARY;
      } else {
          Serial.println("OLED I2C not found at 0x3C or 0x3D");
          oledReady = false;
          return false;
      }
  }

  Serial.printf("Calling g_oled.begin() at 0x%02X\n", oledAddress);

  if (!g_oled.begin(oledAddress, true)) {
      Serial.printf("OLED begin() failed at 0x%02X\n", oledAddress);
      oledReady = false;
      return false;
  }

  delay(100);  // ← add this, let display fully initialize before writing
  Wire.setClock(400000);  // ← moved after begin(), now bump speed

  Serial.println("OLED init success");
  oledReady = true;
  g_oled.fillScreen(SH110X_WHITE);
  g_oled.display();
  delay(500);
  g_oled.fillScreen(SH110X_BLACK);
  g_oled.display();
  delay(500);

  g_oled.clearDisplay();
  g_oled.setTextSize(1);
  g_oled.setTextColor(SH110X_WHITE);
  g_oled.setCursor(0, 0);
  g_oled.println("Booting..");
  g_oled.display();
  delay(1000);
  g_oled.clearDisplay();
  g_oled.setTextSize(1);
  g_oled.setTextColor(SH110X_WHITE);
  g_oled.setCursor(0, 0);
  g_oled.println("Booting...");
  g_oled.display();
  delay(2000);

  g_oled.clearDisplay();
  g_oled.setTextSize(1);
  g_oled.setTextColor(SH110X_WHITE);
  g_oled.setCursor(0, 0);
  g_oled.println("ZypheraOS initialized");
  g_oled.display();
  delay(2000);

  return true;
}

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

void renderDisplay() {
  if (!oledReady) {
    return;
  }

  char selfLine[24];
  char peerLine[24];

  std::snprintf(selfLine, sizeof(selfLine), "Self: %s", selfNodeId);
  std::snprintf(peerLine, sizeof(peerLine), "Peers: %u", static_cast<unsigned>(activePeerCount()));

  g_oled.clearDisplay();
  g_oled.setTextSize(1);
  g_oled.setTextColor(SH110X_WHITE);
  g_oled.setCursor(0, 0);
  g_oled.println(F("ZypheraMesh"));
  g_oled.println(selfLine);
  g_oled.println(peerLine);
  g_oled.println(oledStatusLine);
  g_oled.println(oledEventLine);
#ifdef BOARD_ESP32_DOIT
  g_oled.println(F("ESP32-DOIT"));
#elif defined(BOARD_ESP32_S3_ZERO)
  g_oled.println(F("ESP32-S3 Zero"));
#else
  g_oled.println(F("Unknown Board"));
#endif
  g_oled.display();
}

// ... renderDisplay() ...

static volatile bool renderPending = false;

void requestRender() {
    renderPending = true;
}

void loopDisplay() {
    if (!renderPending) return;
    renderPending = false;
    renderDisplay();
}