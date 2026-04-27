#include "Input/status_button.h"

#include <Arduino.h>

#include "Config/app_config.h"
#include "Display/oled_ui.h"
#include "Status/status_ping.h"

static bool lastButtonReading = LOW;
static bool buttonState = LOW;
static uint32_t lastButtonChangeMs = 0;
static bool lastBadReading = LOW;
static bool badState = LOW;
static uint32_t lastBadChangeMs = 0;

static void broadcastStatus(const char* label, const char* displayStatus, const char* displayEvent, bool sendBad) {
  Serial.printf("Status input -> broadcasting status: %s\n", label);
  setDisplayStatus(displayStatus);
  setDisplayEvent(displayEvent);
  renderDisplay();

  if (sendBad) {
    sendBadStatusPing();
    return;
  }

  sendGoodStatusPing();
}

void handleStatusInputs() {
  const bool reading = digitalRead(STATUS_BUTTON_PIN);
  const bool badReading = digitalRead(BAD_STATUS_PIN);

  if (reading != lastButtonReading) {
    lastButtonChangeMs = millis();
    lastButtonReading = reading;
  }

  if (badReading != lastBadReading) {
    lastBadChangeMs = millis();
    lastBadReading = badReading;
  }

  if (millis() - lastButtonChangeMs >= BUTTON_DEBOUNCE_MS && reading != buttonState) {
    buttonState = reading;
    if (buttonState == LOW) {
      broadcastStatus("Good", "Broadcast Good", "Good input", false);
    }
  }

  if (millis() - lastBadChangeMs < BUTTON_DEBOUNCE_MS || badReading == badState) {
    return;
  }

  badState = badReading;
  if (badState == LOW) {
    broadcastStatus("Bad", "Broadcast Bad", "Bad input", true);
  }
}
