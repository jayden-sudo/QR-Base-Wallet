#ifndef CRC32_H
#define CRC32_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
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

#ifdef __cplusplus
}
#endif

#endif
