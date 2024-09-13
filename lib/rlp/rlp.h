#pragma once

#include <stdlib.h>

typedef struct
{
    uint8_t *data;
    size_t length;
} ByteArray;

typedef struct
{
    ByteArray *items;
    size_t count;
    size_t capacity;
} List;

typedef struct
{
    union
    {
        ByteArray bytes;
        List list;
    } data;
    int is_list;
} RLPItem;

typedef struct
{
    RLPItem item;
    uint8_t *remainder;
    size_t remainder_length;
    int error;
} DecodeResult;

DecodeResult rlp_decode(uint8_t *input, size_t input_length);
void free_decode_result(DecodeResult *result);

int append_signature(
    char *transaction_serialized,
    size_t transaction_serialized_len,
    int yParity,
    char r[32],
    char s[32],
    char *signature,
    const size_t signature_len);