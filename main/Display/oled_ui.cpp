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
#include "CrashLog/CrashLogger.h"

// For reading the crash log file directly
#include <LittleFS.h>

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

static bool pendingRender = false;

void requestRender() {
  pendingRender = true;
}

void loopDisplay() {
  if (pendingRender) {
    pendingRender = false;
    renderDisplay();
  }
}

void showCrashLogOnDisplay() {
  if (!oledReady) return;
  if (!LittleFS.exists(CRASH_LOG_FILE)) return;

  File file = LittleFS.open(CRASH_LOG_FILE, FILE_READ);
  if (!file) return;

  // Read repeat count if present
  uint32_t repeatCount = 0;
  if (LittleFS.exists(CRASH_COUNT_FILE)) {
    File cf = LittleFS.open(CRASH_COUNT_FILE, FILE_READ);
    if (cf) {
      repeatCount = cf.parseInt();
      cf.close();
    }
  }

  // Header screen
  g_oled.clearDisplay();
  g_oled.setTextSize(1);
  g_oled.setTextColor(SH110X_WHITE);
  g_oled.setCursor(0, 0);
  g_oled.println(F("!! CRASH LOG !!"));
  g_oled.println(F(""));
  g_oled.printf("%u bytes\n", (unsigned)file.size());
  if (repeatCount > 0) {
    g_oled.printf("Repeated x%lu\n", (unsigned long)repeatCount);
  }
  g_oled.println(F(""));
  g_oled.println(F("Showing in 2s..."));
  g_oled.display();
  delay(2000);

  // Page through the log — 7 lines per screen (1 reserved for page indicator)
  static const uint8_t LINES_PER_PAGE = 7;
  static const uint8_t LINE_BUF = 22;  // 21 chars + null

  char lines[LINES_PER_PAGE][LINE_BUF];
  uint8_t lineCount = 0;
  uint32_t pageNum = 1;

  auto flushPage = [&]() {
    g_oled.clearDisplay();
    g_oled.setTextSize(1);
    g_oled.setTextColor(SH110X_WHITE);
    g_oled.setCursor(0, 0);
    // Top row: page indicator
    g_oled.printf("-- pg %lu --\n", pageNum++);
    for (uint8_t i = 0; i < lineCount; i++) {
      g_oled.println(lines[i]);
    }
    g_oled.display();
    delay(3000);
    lineCount = 0;
  };

  char ch;
  uint8_t col = 0;
  char lineBuf[LINE_BUF];
  uint8_t lineBufPos = 0;

  auto commitLine = [&]() {
    lineBuf[lineBufPos] = '\0';
    // Skip blank lines to save screen space
    if (lineBufPos > 0) {
      std::strncpy(lines[lineCount++], lineBuf, LINE_BUF - 1);
      lines[lineCount - 1][LINE_BUF - 1] = '\0';
      if (lineCount >= LINES_PER_PAGE) {
        flushPage();
      }
    }
    lineBufPos = 0;
    col = 0;
  };

  while (file.available()) {
    ch = (char)file.read();
    if (ch == '\n' || ch == '\r') {
      if (ch == '\n') commitLine();
    } else {
      if (col < LINE_BUF - 1) {
        lineBuf[lineBufPos++] = ch;
        col++;
      }
      // Long lines wrap automatically by character limit
    }
  }
  // Flush any remaining partial line and partial page
  if (lineBufPos > 0) commitLine();
  if (lineCount > 0) flushPage();

  file.close();

  // Clear both the log and the repeat counter now that it's been shown
  LittleFS.remove(CRASH_LOG_FILE);
  LittleFS.remove(CRASH_COUNT_FILE);

  // Done screen
  g_oled.clearDisplay();
  g_oled.setTextSize(1);
  g_oled.setTextColor(SH110X_WHITE);
  g_oled.setCursor(0, 0);
  g_oled.println(F("-- End of log --"));
  g_oled.println(F("Log cleared."));
  g_oled.println(F("Resuming boot..."));
  g_oled.display();
  delay(1500);
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
  g_oled.setCursor(0, 32);
  g_oled.println(oledStatusLine);
  g_oled.println(oledEventLine);
  g_oled.println("v3.0.1");
  g_oled.display();
}

