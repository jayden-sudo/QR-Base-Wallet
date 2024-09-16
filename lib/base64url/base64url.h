
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <WString.h>

size_t decode_base64url(const String encoded, uint8_t *output, size_t output_len);