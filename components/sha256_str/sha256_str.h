#ifndef SHA256_STR_H
#define SHA256_STR_H

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**********************
     * GLOBAL PROTOTYPES
     **********************/
    void sha256_str(const char *str, uint8_t output[32]);

#ifdef __cplusplus
}
#endif

#endif /* SHA256_STR_H */