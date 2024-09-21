#include "rlp.hpp"

/*
    Encode example:


    struct RLP_ITEM **item_array = new struct RLP_ITEM *[12];
    {
        item_array[0] = new RLP_ITEM{
            .type = RLP_ITEM_BYTES,
            .content = transaction->chainId,
            .content_len = transaction->chainIdLen,
        };
        item_array[1] = new RLP_ITEM{
            .type = RLP_ITEM_BYTES,
            .content = transaction->nonce,
            .content_len = transaction->nonceLen,
        };
        item_array[2] = new RLP_ITEM{
            .type = RLP_ITEM_BYTES,
            .content = transaction->maxPriorityFeePerGas,
            .content_len = transaction->maxPriorityFeePerGasLen,
        };
        item_array[3] = new RLP_ITEM{
            .type = RLP_ITEM_BYTES,
            .content = transaction->maxFeePerGas,
            .content_len = transaction->maxFeePerGasLen,
        };
        item_array[4] = new RLP_ITEM{
            .type = RLP_ITEM_BYTES,
            .content = transaction->gasLimit,
            .content_len = transaction->gasLimitLen,
        };
        item_array[5] = new RLP_ITEM{
            .type = RLP_ITEM_BYTES,
            .content = transaction->to,
            .content_len = transaction->toLen,
        };
        item_array[6] = new RLP_ITEM{
            .type = RLP_ITEM_BYTES,
            .content = transaction->value,
            .content_len = transaction->valueLen,
        };
        item_array[7] = new RLP_ITEM{
            .type = RLP_ITEM_BYTES,
            .content = transaction->data,
            .content_len = transaction->dataLen,
        };
        item_array[8] = new RLP_ITEM{
            .type = RLP_ITEM_LIST,
            .content = transaction->accessList,
            .content_len = transaction->accessListLen,
        };
        item_array[9] = new RLP_ITEM{
            .type = RLP_ITEM_BYTES,
            .content = yParity,
            .content_len = yParity_num
        };
        item_array[10] = new RLP_ITEM{
            .type = RLP_ITEM_BYTES,
            .content = r,
            .content_len = 32,
        };
        item_array[11] = new RLP_ITEM{
            .type = RLP_ITEM_BYTES,
            .content = s,
            .content_len = 32,
        };
        }

        size_t signed_transaction_max_len = transaction_serialized_len + 100;
        *signed_transaction = new uint8_t[signed_transaction_max_len];
        (*signed_transaction)[0] = transaction->transactionType;
        size_t total_len = rlp_encode_array((*signed_transaction) + 1, signed_transaction_max_len - 1, item_array, 12);
        delete[] item_array;
        return total_len + 1;
*/

size_t bytes_to_int(const unsigned char *bytes, size_t length)
{
    size_t result = 0;
    for (size_t i = 0; i < length && i < sizeof(size_t); i++)
    {
        result = (result << 8) | bytes[i];
    }
    return result;
}

size_t int_to_bytes(size_t input, unsigned char resultArray[5])
{
    size_t i = 5;
    while (input > 0)
    {
        i--;
        resultArray[i] = input & 0xff;
        input >>= 8;
    }
    return i;
}

void rlp_decode(uint8_t *input, size_t input_len, struct RLP_ITEM *item)
{
    uint8_t first_byte = input[0];

    if (first_byte <= 0x7f)
    {
        item->type = RLP_ITEM_BYTES;
        item->content_offset = 1;
        item->content_len = 1;
    }
    else if (first_byte <= 0xb7)
    {
        item->type = RLP_ITEM_BYTES;
        item->content_offset = 1;
        item->content_len = first_byte - 0x80;
    }
    // else if (first_byte <= 0xc0)
    // {
    //     item->type = RLP_ITEM_BYTES;
    //     size_t content_arr_len = first_byte - 0xb7;
    //     item->content_offset = 1 + content_arr_len;
    //     item->content_len = bytes_to_int(input + 1, content_arr_len);
    // }
    else if (first_byte <= 0xbf)
    {
        item->type = RLP_ITEM_BYTES;
        size_t content_arr_len = first_byte - 0xb7;
        item->content_offset = 1 + content_arr_len;
        item->content_len = bytes_to_int(input + 1, content_arr_len);
    }
    else if (first_byte <= 0xf7)
    {
        item->type = RLP_ITEM_LIST;
        item->content_offset = 1;
        item->content_len = first_byte - 0xc0;
    }
    else
    {
        item->type = RLP_ITEM_LIST;
        size_t content_arr_len = first_byte - 0xf7;
        item->content_offset = 1 + content_arr_len;
        item->content_len = bytes_to_int(input + 1, content_arr_len);
    }

    item->content = input + item->content_offset;

    if (input_len < (item->content_offset + item->content_len))
    {
        item->type = RLP_ITEM_NULL;
    }
}

#define __RLP_ENCODE_ARRAY_LEN_CHECK \
    do                               \
    {                                \
        if (total_len > buf_len)     \
        {                            \
            return 0;                \
        }                            \
    } while (0)

static int rlp_encode_array_content(uint8_t *buf, size_t buf_len, struct RLP_ITEM **item_array, size_t item_array_size)
{
    uint8_t *_buf = buf;
    size_t total_len = 0;
    for (size_t i = 0; i < item_array_size; i++)
    {
        struct RLP_ITEM *item = item_array[i];
        if (item->type == RLP_ITEM_BYTES)
        {
            if (item->content_len == 1 && item->content[0] <= 0x7f)
            {
                total_len++;
                __RLP_ENCODE_ARRAY_LEN_CHECK;
                memcpy(_buf, item->content, 1);
                _buf++;
            }
            else if (item->content_len < 55)
            {
                total_len += 1;
                __RLP_ENCODE_ARRAY_LEN_CHECK;
                _buf[0] = (0x80 + item->content_len);
                _buf++;
                total_len += item->content_len;
                __RLP_ENCODE_ARRAY_LEN_CHECK;
                memcpy(_buf, item->content, item->content_len);
                _buf += item->content_len;
            }
            else
            {

                unsigned char resultArray[5];
                size_t i = int_to_bytes(item->content_len, resultArray);
                // array length
                size_t arrayLength = 5 - i;
                resultArray[i - 1] = (0xb7 + arrayLength);
                arrayLength += 1;
                total_len += arrayLength;
                __RLP_ENCODE_ARRAY_LEN_CHECK;
                memcpy(_buf, resultArray + (5 - arrayLength), arrayLength);
                _buf += arrayLength;
                total_len += item->content_len;
                __RLP_ENCODE_ARRAY_LEN_CHECK;
                memcpy(_buf, item->content, item->content_len);
                _buf += item->content_len;
            }
        }
        else if (item->type == RLP_ITEM_LIST)
        {
            if (item->content_len < 55)
            {
                total_len++;
                __RLP_ENCODE_ARRAY_LEN_CHECK;
                _buf[0] = (0xc0 + item->content_len);
                _buf++;
                total_len += item->content_len;
                __RLP_ENCODE_ARRAY_LEN_CHECK;
                memcpy(_buf, item->content, item->content_len);
                _buf += item->content_len;
            }
            else
            {
                unsigned char resultArray[5];
                size_t i = int_to_bytes(item->content_len, resultArray);
                // array length
                size_t arrayLength = 5 - i;
                resultArray[i - 1] = (0xf7 + arrayLength);
                arrayLength += 1;
                total_len += arrayLength;
                __RLP_ENCODE_ARRAY_LEN_CHECK;
                memcpy(_buf, resultArray + (5 - arrayLength), arrayLength);
                _buf += arrayLength;
                total_len += item->content_len;
                __RLP_ENCODE_ARRAY_LEN_CHECK;
                memcpy(_buf, item->content, item->content_len);
                _buf += item->content_len;
            }
        }
        else
        {
            // ERROR
            return 0;
        }
    }
    return total_len;
}

int rlp_encode_array(uint8_t *buf, size_t buf_len, struct RLP_ITEM **item_array, size_t item_array_size)
{
    uint8_t *_buf = buf;
    uint8_t *__buf = new uint8_t[buf_len];
    size_t total_len = 0;
    size_t __content_len = rlp_encode_array_content(__buf, buf_len, item_array, item_array_size);
    if (__content_len == 0)
    {
        delete[] __buf;
        return 0;
    }

    if (__content_len < 55)
    {
        total_len++;
        __RLP_ENCODE_ARRAY_LEN_CHECK;
        _buf[0] = (0xc0 + __content_len);
        _buf++;
        total_len += __content_len;
        __RLP_ENCODE_ARRAY_LEN_CHECK;
        memcpy(_buf, __buf, __content_len);
        _buf += __content_len;
    }
    else
    {
        unsigned char resultArray[5];
        size_t i = int_to_bytes(__content_len, resultArray);
        // array length
        size_t arrayLength = 5 - i;
        resultArray[i - 1] = (0xf7 + arrayLength);
        arrayLength += 1;
        total_len += arrayLength;
        __RLP_ENCODE_ARRAY_LEN_CHECK;
        memcpy(_buf, resultArray + (5 - arrayLength), arrayLength);
        _buf += arrayLength;
        total_len += __content_len;
        __RLP_ENCODE_ARRAY_LEN_CHECK;
        memcpy(_buf, __buf, __content_len);
        _buf += __content_len;
    }
    delete[] __buf;
    return total_len;
}