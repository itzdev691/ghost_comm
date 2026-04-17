#pragma once

#include <cstddef>
#include <cstdint>

// Channel and discovery timing (must match across all nodes).
static constexpr uint8_t ESPNOW_CHANNEL = 6;
static constexpr uint32_t DISCOVERY_INTERVAL_MS = 5000;

// GPIO pins — overrides via build_flags per board.
#ifndef STATUS_BUTTON_PIN
#define STATUS_BUTTON_PIN 12
#endif

#if !defined(OLED_SDA_PIN) || !defined(OLED_SCL_PIN)
#if defined(CONFIG_IDF_TARGET_ESP32C5)
#define OLED_SDA_PIN 2
#define OLED_SCL_PIN 3
#else
#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 22
#endif
#endif

static constexpr uint32_t BUTTON_DEBOUNCE_MS = 50;

static constexpr uint8_t OLED_I2C_ADDRESS_PRIMARY = 0x3C;
static constexpr uint8_t OLED_I2C_ADDRESS_SECONDARY = 0x3D;
static constexpr uint8_t OLED_WIDTH = 128;
static constexpr uint8_t OLED_HEIGHT = 64;
static constexpr int8_t OLED_RESET_PIN = -1;

static constexpr size_t MAX_PEERS = 10;
static constexpr uint32_t PEER_TIMEOUT_MS = 30000;
