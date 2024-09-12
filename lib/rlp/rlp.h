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
