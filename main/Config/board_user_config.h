#pragma once

// =============================================================================
// board_user_config.h — Edit this file to match your hardware
// =============================================================================
// This is the ONLY file you need to edit before flashing.
// No build tool configuration required.
// =============================================================================


// -----------------------------------------------------------------------------
// STEP 1: Select your board (uncomment exactly one)
// -----------------------------------------------------------------------------

// #define BOARD_ESP32_DOIT          // ESP32 DOIT DevKit v1
// #define BOARD_ESP32_S3_ZERO       // Waveshare ESP32-S3 Zero
#define BOARD_ESP32_C5_DEV           // ESP32-C5 DevKitC-1 (default)
// #define BOARD_GENERIC_ESP32       // Any other ESP32 classic board


// -----------------------------------------------------------------------------
// STEP 2: OLED I2C pins
// These are set automatically based on your board selection above.
// Override here only if your wiring differs from the defaults.
// -----------------------------------------------------------------------------

#if !defined(OLED_SDA_PIN) || !defined(OLED_SCL_PIN)

#if defined(BOARD_ESP32_DOIT)
  #define OLED_SDA_PIN 13
  #define OLED_SCL_PIN 27

#elif defined(BOARD_ESP32_S3_ZERO)
  #define OLED_SDA_PIN 5
  #define OLED_SCL_PIN 6

#elif defined(BOARD_ESP32_C5_DEV)
  #define OLED_SDA_PIN 2
  #define OLED_SCL_PIN 3

#else
  // Generic ESP32 classic fallback — verify against your PCB datasheet
  #define OLED_SDA_PIN 21
  #define OLED_SCL_PIN 22

#endif
#endif // !defined(OLED_SDA_PIN) || !defined(OLED_SCL_PIN)


// -----------------------------------------------------------------------------
// STEP 3: Button GPIO pins
// These are set automatically based on your board selection above.
// Override here only if your wiring differs from the defaults.
// -----------------------------------------------------------------------------

#if !defined(STATUS_BUTTON_PIN)

#if defined(BOARD_ESP32_DOIT)
  #define STATUS_BUTTON_PIN 12

#elif defined(BOARD_ESP32_S3_ZERO)
  #define STATUS_BUTTON_PIN 10

#elif defined(BOARD_ESP32_C5_DEV)
  #define STATUS_BUTTON_PIN 10

#else
  #define STATUS_BUTTON_PIN 12

#endif
#endif // !defined(STATUS_BUTTON_PIN)


// -----------------------------------------------------------------------------
// STEP 4: Onboard LED
// Set automatically. Override here if needed.
// -----------------------------------------------------------------------------

#if !defined(ONBOARD_LED_PIN)

#if defined(BOARD_ESP32_S3_ZERO)
  #define ONBOARD_LED_PIN 21
  #define ONBOARD_LED_USES_NEOPIXEL 1

#elif defined(BOARD_ESP32_DOIT)
  #define ONBOARD_LED_PIN 2
  #define ONBOARD_LED_USES_NEOPIXEL 0

#elif defined(BOARD_ESP32_C5_DEV)
  #define ONBOARD_LED_PIN 8
  #define ONBOARD_LED_USES_NEOPIXEL 0

#else
  #ifdef LED_BUILTIN
    #define ONBOARD_LED_PIN LED_BUILTIN
  #else
    #define ONBOARD_LED_PIN 2
  #endif
  #define ONBOARD_LED_USES_NEOPIXEL 0

#endif
#endif // !defined(ONBOARD_LED_PIN)
