#pragma once

#include <stdint.h>

bool initDisplay();
void renderDisplay();

void setDisplayStatus(const char* message);
void setDisplayEvent(const char* message);
void setDisplayEventFromMac(const char* label, const uint8_t* macAddr);
void setDisplayEventFromPeer(const char* label, const uint8_t* macAddr);
