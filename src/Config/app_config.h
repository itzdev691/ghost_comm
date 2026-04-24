#pragma once

#include <cstddef>
#include <cstdint>

#include "Config/board_user_config.h"

// Channel and discovery timing (must match across all nodes).
static constexpr uint8_t ESPNOW_CHANNEL = 6;
static constexpr uint32_t DISCOVERY_INTERVAL_MS = 5000;

// GPIO pins — overrides via build_flags per board (optional).
#ifndef STATUS_BUTTON_PIN
#define STATUS_BUTTON_PIN 12
#endif

#ifndef ONBOARD_LED_PIN
#if defined(LED_BUILTIN)
#define ONBOARD_LED_PIN LED_BUILTIN
#elif defined(BOARD_ESP32_DOIT)
#define ONBOARD_LED_PIN 2
#else
#define ONBOARD_LED_PIN 2
#endif
#endif

#ifndef ONBOARD_LED_ACTIVE_HIGH
#define ONBOARD_LED_ACTIVE_HIGH 1
#endif

static constexpr uint32_t BUTTON_DEBOUNCE_MS = 50;
static constexpr uint32_t RX_LED_HOLD_MS = 250;

static constexpr uint8_t OLED_I2C_ADDRESS_PRIMARY = 0x3C;
static constexpr uint8_t OLED_I2C_ADDRESS_SECONDARY = 0x3D;
static constexpr uint8_t OLED_WIDTH = 128;
static constexpr uint8_t OLED_HEIGHT = 64;
static constexpr int8_t OLED_RESET_PIN = -1;

static constexpr size_t MAX_PEERS = 10;
static constexpr uint32_t PEER_TIMEOUT_MS = 30000;
