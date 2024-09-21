#include "base64url.hpp"
#include <string>
#include <Conversion.h>
#include <algorithm>

size_t decode_base64url(const std::string encoded, uint8_t *output,
                        size_t output_len)
{
    std::string encoded_url = encoded;

    std::replace(encoded_url.begin(), encoded_url.end(), '-', '+');
    std::replace(encoded_url.begin(), encoded_url.end(), '_', '/');

    return fromBase64(encoded_url, output, output_len, BASE64_NOPADDING);
}