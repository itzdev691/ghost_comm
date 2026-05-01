#pragma once

#include <cstddef>
#include <cstdint>

#include "Config/board_user_config.h"

// Channel and discovery timing (must match across all nodes).
static constexpr uint8_t ESPNOW_CHANNEL = 6;
static constexpr uint32_t DISCOVERY_INTERVAL_MS = 5000;
static constexpr uint8_t DEFAULT_FORWARD_TTL = 3;
static constexpr uint32_t FORWARD_DELAY_MIN_MS = 25;
static constexpr uint32_t FORWARD_DELAY_MAX_MS = 90;
static constexpr size_t SEEN_MESSAGE_CACHE_SIZE = 24;


// GPIO pins — all defined in board_user_config.h, edit that file for your hardware.

#ifndef BAD_STATUS_PIN
#define BAD_STATUS_PIN 9
#endif

#ifndef ONBOARD_LED_ACTIVE_HIGH
#define ONBOARD_LED_ACTIVE_HIGH 1
#endif

static constexpr uint8_t ONBOARD_LED_RX_RED = 0;
static constexpr uint8_t ONBOARD_LED_RX_GREEN = 24;
static constexpr uint8_t ONBOARD_LED_RX_BLUE = 0;

static constexpr uint32_t BUTTON_DEBOUNCE_MS = 50;
static constexpr uint32_t RX_LED_HOLD_MS = 250;
static constexpr uint32_t DISCOVERY_ACK_DELAY_MS = 150;

static constexpr uint8_t OLED_I2C_ADDRESS_PRIMARY = 0x3C;
static constexpr uint8_t OLED_I2C_ADDRESS_SECONDARY = 0x3D;
static constexpr uint8_t OLED_WIDTH = 128;
static constexpr uint8_t OLED_HEIGHT = 64;
static constexpr int8_t OLED_RESET_PIN = -1;

static constexpr size_t MAX_PEERS = 10;
static constexpr uint32_t PEER_TIMEOUT_MS = 30000;
