#pragma once

#include <string>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include "crc32.h"

std::string encode_minimal(uint8_t *source, size_t sourceLen);

int decode_minimal(
    std::string source,
    uint8_t **output_payload);
