#ifndef AES_H
#define AES_H

#include <stdint.h>
#include <stddef.h>

#define AES_BLOCK_SIZE 16

#ifdef __cplusplus
extern "C"
{
#endif

    int aes_encrypt(const unsigned char key[32], const unsigned char *plaintext,
                    size_t len, unsigned char *ciphertext);
    int aes_decrypt(const unsigned char key[32], const unsigned char *ciphertext,
                    size_t len, unsigned char *plaintext);

#ifdef __cplusplus
}
#endif

#endif /* AES_H */
