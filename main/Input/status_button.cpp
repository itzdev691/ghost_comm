#include "Input/status_button.h"

#include <Arduino.h>

#include "Config/app_config.h"
#include "Display/oled_ui.h"
#include "Status/status_ping.h"

static bool lastButtonReading = LOW;
static bool buttonState = LOW;
static uint32_t lastButtonChangeMs = 0;
static uint32_t buttonPressStartedMs = 0;

static void broadcastStatus(const char* label, const char* displayStatus, const char* displayEvent, bool sendBad) {
  Serial.printf("Button action -> broadcasting status: %s\n", label);
  setDisplayStatus(displayStatus);
  setDisplayEvent(displayEvent);
  renderDisplay();

  if (sendBad) {
    sendBadStatusPing();
    return;
  }

  sendGoodStatusPing();
}

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
    buttonPressStartedMs = millis();
    return;
  }

  uint32_t heldMs = millis() - buttonPressStartedMs;
  buttonPressStartedMs = 0;

  if (heldMs >= STATUS_BAD_HOLD_MS) {
    broadcastStatus("Bad", "Broadcast Bad", "Long press", true);
    return;
  }

  broadcastStatus("Good", "Broadcast Good", "Short press", false);
}
