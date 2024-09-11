#pragma once

#include <stdint.h>

#if defined(ESP32)
  #include <esp_system.h>
  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
    #include <esp32/rom/crc.h>
  #else
   #error "ESP32 CRC32 not supported"
  #endif
#else
  #error "ESP32 CRC32 not supported"
#endif

/**
 * @brief CRC32 value that is in little endian.
 *
 * @param  uint32_t crc : init crc value, use 0 at the first use.
 *
 * @param  uint8_t const *buf : buffer to start calculate crc.
 *
 * @param  uint32_t len : buffer length in byte.
 *
 * @return None
 */
uint32_t crc32(uint32_t crc, uint8_t const *buf, uint32_t len);