#pragma once

// ---------------------------------------------------------------------------
// User-editable OLED I2C GPIO (PlatformIO-independent)
//
// Edit the values in the final #else branch, or add #elif defined(YOUR_BOARD)
// with your SDA/SCL pins. Pinout must match your hardware.
//
// Resolution: optional compile-time -DOLED_SDA_PIN / -DOLED_SCL_PIN (any build
// system) overrides this file. Otherwise known boards use the branches below,
// then the generic fallback.
// ---------------------------------------------------------------------------

#if !defined(OLED_SDA_PIN) || !defined(OLED_SCL_PIN)

#if defined(BOARD_ESP32_DOIT)
#define OLED_SDA_PIN 13
#define OLED_SCL_PIN 27

#elif defined(BOARD_ESP32_S3_ZERO)
#define OLED_SDA_PIN 6
#define OLED_SCL_PIN 7

#elif defined(CONFIG_IDF_TARGET_ESP32C5) || defined(ARDUINO_ESP32C5_DEV)
#define OLED_SDA_PIN 6
#define OLED_SCL_PIN 7

#else
// Generic ESP32 classic: common module defaults — verify against your PCB.
#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 22

#endif

#endif  // !defined(OLED_SDA_PIN) || !defined(OLED_SCL_PIN)
