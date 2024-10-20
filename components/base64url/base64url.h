#ifndef BASE64URL_H
#define BASE64URL_H

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**********************
     * GLOBAL PROTOTYPES
     **********************/
    size_t decode_base64url(const char *encoded, uint8_t *output, size_t output_max_len);

#ifdef __cplusplus
}
#endif

#endif /* BASE64URL_H */