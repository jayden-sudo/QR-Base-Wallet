#include "aes.h"
#include <mbedtls/aes.h>
#include <string.h>
#include <errno.h>

static inline int aes_set_key(mbedtls_aes_context *ctx, const unsigned char *key)
{
    return mbedtls_aes_setkey_enc(ctx, key, 256);
}

static int aes_crypt_block(mbedtls_aes_context *ctx, int mode,
                           const unsigned char *input, unsigned char *output)
{
    return mbedtls_aes_crypt_ecb(ctx, mode, input, output);
}

int aes_crypt_ecb(const unsigned char *key, int mode,
                  const unsigned char input[16], unsigned char output[16])
{
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);

    int ret = aes_set_key(&ctx, key);
    if (ret)
        goto error;

    ret = aes_crypt_block(&ctx, mode, input, output);
error:
    mbedtls_aes_free(&ctx);

    return ret;
}

int AES_encrypt(const unsigned char key[32], const unsigned char *plaintext,
                size_t len, unsigned char *ciphertext)
{
    if (len % AES_BLOCK_SIZE)
        return EINVAL;

    int ret = 0;
    for (size_t i = 0; i < len; i += AES_BLOCK_SIZE)
    {
        ret = aes_crypt_ecb(key, MBEDTLS_AES_ENCRYPT,
                            plaintext + i, ciphertext + i);
        if (ret)
        {
            return ret;
        }
    }

    return 0;
}

int AES_decrypt(const unsigned char key[32], const unsigned char *ciphertext,
                size_t len, unsigned char *plaintext)
{

    if (len % AES_BLOCK_SIZE)
        return EINVAL;
    int ret = 0;
    for (size_t i = 0; i < len; i += AES_BLOCK_SIZE)
    {
        ret = aes_crypt_ecb(key, MBEDTLS_AES_DECRYPT,
                            ciphertext + i, plaintext + i);
        if (ret)
        {
            return ret;
        }
    }

    return 0;
}