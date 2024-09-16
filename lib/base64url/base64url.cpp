#include "base64url.h"
#include <Conversion.h>

size_t decode_base64url(const String encoded, uint8_t *output, size_t output_len)
{
     String encoded_url = encoded;
     encoded_url.replace("-", "+");
     encoded_url.replace("_", "/");
     return fromBase64(encoded_url, output, output_len, BASE64_NOPADDING);
}