#pragma once

// ---------------------------------------------------------------------------
// User-editable OLED I2C GPIO (PlatformIO-independent)
//
// CUSTOMIZATION OPTIONS (in priority order):
// 1. Add -DOLED_SDA_PIN=X -DOLED_SCL_PIN=Y to your build flags
// 2. Add #elif defined(YOUR_BOARD) branch below with your pins
// 3. Edit the generic #else fallback pins (currently 21/22)
//
// Pinout must match your hardware. Verify against your PCB datasheet.
// ---------------------------------------------------------------------------

#if !defined(OLED_SDA_PIN) || !defined(OLED_SCL_PIN)

#if defined(BOARD_ESP32_DOIT)
#define OLED_SDA_PIN 13
#define OLED_SCL_PIN 27

#elif defined(BOARD_ESP32_S3_ZERO)
#define OLED_SDA_PIN 8
#define OLED_SCL_PIN 9

#elif defined(CONFIG_IDF_TARGET_ESP32C5) || defined(ARDUINO_ESP32C5_DEV)
#define OLED_SDA_PIN 2
#define OLED_SCL_PIN 3

// *** ADD YOUR CUSTOM BOARD HERE ***
// Example:
// #elif defined(MY_CUSTOM_BOARD)
// #define OLED_SDA_PIN 18
// #define OLED_SCL_PIN 19

#else
// Generic ESP32 classic: common module defaults — verify against your PCB.
// EDIT THESE if using a non-standard pinout:
#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 22

#endif

#endif  // !defined(OLED_SDA_PIN) || !defined(OLED_SCL_PIN)