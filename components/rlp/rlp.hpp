#pragma once
#include <string>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#define RLP_ITEM_NULL 0
#define RLP_ITEM_LIST 1
#define RLP_ITEM_BYTES 2

struct RLP_ITEM
{
    /*
        RLP_ITEM_NULL, RLP_ITEM_LIST, RLP_ITEM_BYTES
     */
    uint8_t type;
    /*
        if item is `RLP_ITEM_LIST`, content is the pointer to the first item in the list
        if item is `RLP_ITEM_BYTES`, content is the pointer to the content of the bytes
     */
    uint8_t *content;
    /*
        if item is `RLP_ITEM_LIST`, content_len is the length of all items in the list
        if item is `RLP_ITEM_BYTES`, content_len is the length of the bytes
     */
    size_t content_len;
    /*
        if item is `RLP_ITEM_LIST`, content_offset is the offset of the first item in the list
        if item is `RLP_ITEM_BYTES`, content_offset is the offset of the bytes
     */
    size_t content_offset;
};

void rlp_decode(uint8_t *input, size_t input_len, struct RLP_ITEM *item);

int rlp_encode_array(uint8_t *buf, size_t buf_len, struct RLP_ITEM **item_array, size_t item_array_size);
