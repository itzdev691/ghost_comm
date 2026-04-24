#include "Status/onboard_led.h"

#include <Arduino.h>

#include "Config/app_config.h"

static bool g_onboardLedReady = false;
static uint32_t g_receiveLedUntilMs = 0;

static void setOnboardLed(bool enabled) {
  const uint8_t inactiveLevel = ONBOARD_LED_ACTIVE_HIGH ? LOW : HIGH;
  const uint8_t activeLevel = ONBOARD_LED_ACTIVE_HIGH ? HIGH : LOW;
  digitalWrite(ONBOARD_LED_PIN, enabled ? activeLevel : inactiveLevel);
}

void initOnboardLed() {
  pinMode(ONBOARD_LED_PIN, OUTPUT);
  setOnboardLed(false);
  g_onboardLedReady = true;
}

void noteReceiveActivity() {
  if (!g_onboardLedReady) {
    return;
  }

  g_receiveLedUntilMs = millis() + RX_LED_HOLD_MS;
  setOnboardLed(true);
}

void loopOnboardLed() {
  if (!g_onboardLedReady || g_receiveLedUntilMs == 0) {
    return;
  }

  if (static_cast<int32_t>(millis() - g_receiveLedUntilMs) < 0) {
    return;
  }

  g_receiveLedUntilMs = 0;
  setOnboardLed(false);
}
