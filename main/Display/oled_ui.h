#pragma once

#include <stdint.h>

bool initDisplay();
void renderDisplay();
void loopDisplay();
void requestRender();

// Reads /crash_log.txt from LittleFS and scrolls it on the OLED.
// Does nothing if no log exists. Call after initDisplay() and crashLogger.begin().
void showCrashLogOnDisplay();

void setDisplayStatus(const char* message);
void setDisplayEvent(const char* message);
void setDisplayEventFromMac(const char* label, const uint8_t* macAddr);
void setDisplayEventFromPeer(const char* label, const uint8_t* macAddr);