#ifndef KV_FS_H
#define KV_FS_H

/*********************
 *      INCLUDES
 *********************/
#include <stddef.h>

 /*********************
 *      DEFINES
 *********************/
#define KV_FS_KEY_WALLET "key_wallet_default"
#ifdef __cplusplus
extern "C"
{
#endif

    /**********************
     * GLOBAL PROTOTYPES
     **********************/
    int kv_save(const char *key, const char *hex, size_t len);
    int kv_load(const char *key, char **hex, size_t *len);
    int kv_delete(const char *key);
#ifdef __cplusplus
}
#endif

#endif // KV_FS_H