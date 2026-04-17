#include "Input/status_button.h"

#include <Arduino.h>

#include "Config/app_config.h"
#include "Display/oled_ui.h"
#include "Status/status_ping.h"

static bool lastButtonReading = LOW;
static bool buttonState = LOW;
static uint32_t lastButtonChangeMs = 0;

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
