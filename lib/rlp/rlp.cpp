#include "rlp.h"
#include <string.h>

#define MAX_DEPTH 100

uint64_t decode_length(uint8_t *data, size_t length);

DecodeResult rlp_decode(uint8_t *input, size_t input_length)
{
    DecodeResult result = {0};
    result.error = 0;
    uint8_t first_byte = input[0];

    if (first_byte <= 0x7f)
    {
        // Single byte
        result.item.is_list = 0;
        result.item.data.bytes.data = input;
        result.item.data.bytes.length = 1;
        result.remainder = input + 1;
        result.remainder_length = input_length - 1;
    }
    else if (first_byte <= 0xb7)
    {
        // Short string
        size_t length = first_byte - 0x80;
        if (input_length < length + 1)
        {
            // Invalid RLP: not enough bytes for short string
            result.error = 1;
            return result;
        }
        result.item.is_list = 0;
        result.item.data.bytes.data = input + 1;
        result.item.data.bytes.length = length;
        result.remainder = input + 1 + length;
        result.remainder_length = input_length - 1 - length;
    }
    else if (first_byte <= 0xbf)
    {
        // Long string
        size_t length_of_length = first_byte - 0xb7;
        if (input_length < 1 + length_of_length)
        {
            // Invalid RLP: not enough bytes for string length
            result.error = 1;
            return result;
        }
        uint64_t length = decode_length(input + 1, length_of_length);
        if (input_length < 1 + length_of_length + length)
        {
            // Invalid RLP: not enough bytes for long string
            result.error = 1;
            return result;
        }
        result.item.is_list = 0;
        result.item.data.bytes.data = input + 1 + length_of_length;
        result.item.data.bytes.length = length;
        result.remainder = input + 1 + length_of_length + length;
        result.remainder_length = input_length - 1 - length_of_length - length;
    }
    else if (first_byte <= 0xf7)
    {
        // Short list
        size_t length = first_byte - 0xc0;
        result.item.is_list = 1;
        result.item.data.list.items = new ByteArray[MAX_DEPTH];
        result.item.data.list.count = 0;
        result.item.data.list.capacity = MAX_DEPTH;

        uint8_t *current = input + 1;
        size_t remaining = length;
        while (remaining > 0)
        {
            DecodeResult inner_result = rlp_decode(current, remaining);
            if (inner_result.error != 0)
            {
                result.error = 1;
                return result;
            }
            if (result.item.data.list.count >= result.item.data.list.capacity)
            {
                // List capacity exceeded
                result.error = 1;
                return result;
            }
            result.item.data.list.items[result.item.data.list.count++] = inner_result.item.data.bytes;
            size_t consumed = inner_result.remainder - current;
            current += consumed;
            remaining -= consumed;
        }
        result.remainder = input + 1 + length;
        result.remainder_length = input_length - 1 - length;
    }
    else
    {
        // Long list
        size_t length_of_length = first_byte - 0xf7;
        if (input_length < 1 + length_of_length)
        {
            // Invalid RLP: not enough bytes for list length
            result.error = 1;
            return result;
        }
        uint64_t length = decode_length(input + 1, length_of_length);
        if (input_length < 1 + length_of_length + length)
        {
            // Invalid RLP: not enough bytes for long list
            result.error = 1;
            return result;
        }
        result.item.is_list = 1;
        result.item.data.list.items = new ByteArray[MAX_DEPTH];
        result.item.data.list.count = 0;
        result.item.data.list.capacity = MAX_DEPTH;

        uint8_t *current = input + 1 + length_of_length;
        size_t remaining = length;
        while (remaining > 0)
        {
            DecodeResult inner_result = rlp_decode(current, remaining);
            if (inner_result.error != 0)
            {
                result.error = 1;
                return result;
            }
            if (result.item.data.list.count >= result.item.data.list.capacity)
            {
                // List capacity exceeded
                result.error = 1;
                return result;
            }
            result.item.data.list.items[result.item.data.list.count++] = inner_result.item.data.bytes;
            size_t consumed = inner_result.remainder - current;
            current += consumed;
            remaining -= consumed;
        }
        result.remainder = input + 1 + length_of_length + length;
        result.remainder_length = input_length - 1 - length_of_length - length;
    }

    return result;
}

uint64_t decode_length(uint8_t *data, size_t length)
{
    uint64_t result = 0;
    for (uint8_t i = 0; i < length; i++)
    {
        result = (result << 8) | data[i];
    }
    return result;
}

void free_decode_result(DecodeResult *result)
{
    if (result->item.is_list)
    {
        delete[] result->item.data.list.items;
        result->item.data.list.items = nullptr;
    }
}

size_t arrayifyInteger(size_t input, char resultArray[5])
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

int append_signature(
    char *transaction_serialized,
    size_t transaction_serialized_len,
    int yParity,
    char r[32],
    char s[32],
    char *signature,
    const size_t signature_len)
{
    // transaction_type
    signature[0] = transaction_serialized[0];
    // rlp header
    int rlp_header1 = transaction_serialized[1];
    int rlp_header_length = 1;
    if (rlp_header1 >= 0xf7)
    {
        // rlp header length
        rlp_header_length += (rlp_header1 - 0xf7);
    }
    char *payload = transaction_serialized + 1 + rlp_header_length;
    size_t payload_len = transaction_serialized_len - (1 + rlp_header_length);

    // append signature
    char rlp_signature[67];
    // append yParity
    // [0]
    rlp_signature[0] = yParity;
    // append r
    // [1]
    rlp_signature[1] = (0x80 + 32);
    // [2,33]
    memcpy(rlp_signature + 2, r, 32);
    // append s
    // [34]
    rlp_signature[34] = (0x80 + 32);
    // [35,66]
    memcpy(rlp_signature + 35, s, 32);

    // payload length
    size_t payload_length = payload_len + 67;
    size_t payload_offset = 0;
    if (payload_length <= 55)
    {
        signature[1] = (0xc0 + payload_length);
        payload_offset = 2;
    }
    else
    {
        char resultArray[5];
        size_t i = arrayifyInteger(payload_length, resultArray);
        // array length
        size_t arrayLength = 5 - i;
        resultArray[i - 1] = (0xf7 + arrayLength);
        arrayLength += 1;
        memcpy(signature + 1, resultArray + (5 - arrayLength), arrayLength);
        payload_offset = 1 + arrayLength;
    }
    if (signature_len < payload_offset + payload_len + 67)
    {
        return 0;
    }

    // copy payload
    memcpy(signature + payload_offset, payload, payload_len);
    // copy signature
    memcpy(signature + payload_offset + payload_len, rlp_signature, 67);

    return payload_offset + payload_len + 67;
}