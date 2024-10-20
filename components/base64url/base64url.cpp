/*********************
 *      INCLUDES
 *********************/
#include "base64url.h"
#include <string>
#include <Conversion.h>
#include <algorithm>

/**********************
 *   STATIC FUNCTIONS
 **********************/
static size_t _decode_base64url(const char *encoded, uint8_t *output,
                                size_t output_len)
{
    std::string encoded_url = std::string(encoded);

    std::replace(encoded_url.begin(), encoded_url.end(), '-', '+');
    std::replace(encoded_url.begin(), encoded_url.end(), '_', '/');

    return fromBase64(encoded_url, output, output_len, BASE64_NOPADDING);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
extern "C" size_t decode_base64url(const char *encoded, uint8_t *output, size_t output_max_len)
{
    size_t len = strlen(encoded);
    if (output_max_len < len)
    {
        return 0;
    }
    size_t decoded_len = _decode_base64url(encoded, output, len);
    return decoded_len;
}
