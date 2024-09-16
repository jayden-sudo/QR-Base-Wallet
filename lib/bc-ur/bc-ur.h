#pragma once

#include <stdint.h>
#include <WString.h>

String encode_minimal(uint8_t *source, size_t sourceLen);

int decode_minimal(
    String source,
    uint8_t **output_payload);