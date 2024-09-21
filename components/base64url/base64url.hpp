#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

size_t decode_base64url(const std::string encoded, uint8_t *output, size_t output_len);