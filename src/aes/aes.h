#pragma once

#include <stddef.h>

#define AES_BLOCK_SIZE 16

int AES_encrypt(const unsigned char key[32], const unsigned char *plaintext, size_t len,unsigned char *ciphertext);
int AES_decrypt(const unsigned char key[32], const unsigned char *ciphertext, size_t len,unsigned char *plaintext);