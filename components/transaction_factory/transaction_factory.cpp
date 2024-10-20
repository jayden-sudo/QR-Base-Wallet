/*********************
 *      INCLUDES
 *********************/
#include "transaction_factory.h"
#include "Conversion.h"
#include "rlp.h"
#include <string>
#include "utility/trezor/bignum.h"
#include "esp_log.h"
#include <string.h>

/*********************
 *      DEFINES
 *********************/
#define TAG "transaction_factory"

/**********************
 *      MACROS
 **********************/
#define BUFFER_TO_HEX(hex_name, byteArray, byteArrayLength)  \
    do                                                       \
    {                                                        \
        for (size_t i = 0; i < byteArrayLength; i++)         \
        {                                                    \
            sprintf(&hex_name[i * 2], "%02x", byteArray[i]); \
        }                                                    \
        hex_name[byteArrayLength * 2] = '\0';                \
    } while (0)

#define BUFFER_TO_BIGNUMBER(input, inputLen, output)     \
    do                                                   \
    {                                                    \
        uint8_t temp[32] = {0};                          \
        memcpy(temp + (32 - inputLen), input, inputLen); \
        bn_read_be(temp, output);                        \
    } while (0)

/**********************
 *  STATIC PROTOTYPES
 **********************/
uint64_t to_uint64_t(uint8_t *input, size_t input_size);
bignum256 to_bignum256(uint8_t *input, size_t input_size);
std::string to_hex_string(uint8_t *input, size_t input_size);

/**********************
 *   STATIC FUNCTIONS
 **********************/
uint64_t to_uint64_t(uint8_t *input, size_t input_size)
{
    bignum256 _data = to_bignum256(input, input_size);
    return bn_write_uint64(&_data);
}
bignum256 to_bignum256(uint8_t *input, size_t input_size)
{
    bignum256 _data;
    BUFFER_TO_BIGNUMBER(input, input_size, &_data);
    return _data;
}
std::string to_hex_string(uint8_t *input, size_t input_size)
{
    std::string _data = toHex(input, input_size);
    return _data;
}

extern "C"
{
    /**********************
     * GLOBAL PROTOTYPES
     **********************/
    void transaction_factory_init(TransactionData *transaction_data, const uint8_t *sign_data, size_t sign_data_len);
    void transaction_factory_free(TransactionData *transaction_data);
    char *transaction_factory_to_string(TransactionData *transaction_data);

    /**********************
     *   GLOBAL FUNCTIONS
     **********************/
    void transaction_factory_init(TransactionData *transaction_data, const uint8_t *sign_data, size_t sign_data_len)
    {
        memset(transaction_data, 0, sizeof(TransactionData));

        if (sign_data[0] <= 0x7f)
        {
            // Ethereum Transaction Type
            switch (sign_data[0])
            {
            case TRANSACTION_TYPE_ACCESS_LIST_EIP2930:
            {
                // not implemented
                ESP_LOGE(TAG, "TRANSACTION_TYPE_ACCESS_LIST_EIP2930 not implemented");
                transaction_data->error = 1;
                return;
            }
            case TRANSACTION_TYPE_FEE_MARKET_EIP1559:
            {
                uint8_t *chainId = nullptr;
                size_t chainIdLen = 0;
                uint8_t *nonce = nullptr;
                size_t nonceLen = 0;
                uint8_t *maxPriorityFeePerGas = nullptr;
                size_t maxPriorityFeePerGasLen = 0;
                uint8_t *maxFeePerGas = nullptr;
                size_t maxFeePerGasLen = 0;
                uint8_t *gasLimit = nullptr;
                size_t gasLimitLen = 0;
                uint8_t *to = nullptr;
                size_t toLen = 0;
                uint8_t *value = nullptr;
                size_t valueLen = 0;
                uint8_t *data = nullptr;
                size_t dataLen = 0;
                uint8_t *accessList = nullptr;
                size_t accessListLen = 0;

                uint8_t *rlp_encoded_ptr = (uint8_t *)sign_data;
                size_t rlp_encoded_len = sign_data_len;

                rlp_encoded_ptr++;
                rlp_encoded_len--;

                struct RLP_ITEM item;

                rlp_decode(rlp_encoded_ptr, rlp_encoded_len, &item);
                if (item.type != RLP_ITEM_LIST)
                {
                    ESP_LOGE(TAG, "Invalid RLP encoded data");
                    transaction_data->error = 1;
                    return;
                }
                rlp_encoded_len = item.content_len;
                rlp_encoded_ptr = item.content;
                // EIP1559 Transaction has 9 items
                const size_t transaction_item_len = 9;
                for (size_t i = 0; i < transaction_item_len; i++)
                {
                    rlp_decode(rlp_encoded_ptr, rlp_encoded_len, &item);
                    switch (i)
                    {
                    case 0:
                    {
                        chainId = item.content;
                        chainIdLen = item.content_len;
                        break;
                    }
                    case 1:
                    {
                        nonce = item.content;
                        nonceLen = item.content_len;
                        break;
                    }
                    case 2:
                    {
                        maxPriorityFeePerGas = item.content;
                        maxPriorityFeePerGasLen = item.content_len;
                        break;
                    }
                    case 3:
                    {
                        maxFeePerGas = item.content;
                        maxFeePerGasLen = item.content_len;
                        break;
                    }
                    case 4:
                    {
                        gasLimit = item.content;
                        gasLimitLen = item.content_len;
                        break;
                    }
                    case 5:
                    {
                        to = item.content;
                        toLen = item.content_len;
                        break;
                    }
                    case 6:
                    {
                        value = item.content;
                        valueLen = item.content_len;
                        break;
                    }
                    case 7:
                    {
                        data = item.content;
                        dataLen = item.content_len;
                        break;
                    }
                    case 8:
                    {
                        accessList = item.content;
                        accessListLen = item.content_len;
                        break;
                    }
                    }
                    if (item.type == RLP_ITEM_NULL)
                    {
                        ESP_LOGE(TAG, "Invalid RLP encoded data");
                        transaction_data->error = 1;
                        return;
                    }
                    else if (item.type == RLP_ITEM_BYTES)
                    {
                        size_t skip_len = item.content_offset + item.content_len;
                        rlp_encoded_ptr += skip_len;
                        rlp_encoded_len -= skip_len;
                    }
                    else if (item.type == RLP_ITEM_LIST)
                    {
                        size_t skip_len = item.content_offset + item.content_len;
                        rlp_encoded_ptr += skip_len;
                        rlp_encoded_len -= skip_len;
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Invalid RLP encoded data");
                        transaction_data->error = 1;
                        return;
                    }
                }

                transaction_data->rlp_encoded = (uint8_t *)malloc(sign_data_len);
                memcpy(transaction_data->rlp_encoded, sign_data, sign_data_len);
                transaction_data->rlp_encoded_len = sign_data_len;
                transaction_data->transactionType = TRANSACTION_TYPE_FEE_MARKET_EIP1559;
                transaction_data->chainId = chainId;
                transaction_data->chainIdLen = chainIdLen;
                transaction_data->nonce = nonce;
                transaction_data->nonceLen = nonceLen;
                transaction_data->maxPriorityFeePerGas = maxPriorityFeePerGas;
                transaction_data->maxPriorityFeePerGasLen = maxPriorityFeePerGasLen;
                transaction_data->maxFeePerGas = maxFeePerGas;
                transaction_data->maxFeePerGasLen = maxFeePerGasLen;
                transaction_data->gasLimit = gasLimit;
                transaction_data->gasLimitLen = gasLimitLen;
                transaction_data->to = to;
                transaction_data->toLen = toLen;
                transaction_data->value = value;
                transaction_data->valueLen = valueLen;
                transaction_data->data = data;
                transaction_data->dataLen = dataLen;
                transaction_data->accessList = accessList;
                transaction_data->accessListLen = accessListLen;
                return;
            }
            case TRANSACTION_TYPE_BLOB_EIP4844:
            {
                // not implemented
                ESP_LOGE(TAG, "TRANSACTION_TYPE_BLOB_EIP4844 not implemented");
                transaction_data->error = 1;
                return;
            }

            case TRANSACTION_TYPE_EOA_CODE_EIP7702:
            {
                // not implemented
                ESP_LOGE(TAG, "TRANSACTION_TYPE_EOA_CODE_EIP7702 not implemented");
                transaction_data->error = 1;
                return;
            }
            default:
            {
                // not implemented
                ESP_LOGE(TAG, "Unknown transaction type");
                transaction_data->error = 1;
                return;
            }
            }
        }
        else
        {
            // TransactionType_Legacy
            // not implemented
            ESP_LOGE(TAG, "TRANSACTION_TYPE_LEGACY not implemented");
            transaction_data->error = 1;
            return;
        }
    }
    void transaction_factory_free(TransactionData *transaction_data)
    {
        if (transaction_data->rlp_encoded)
        {
            free(transaction_data->rlp_encoded);
        }
    }
    char *transaction_factory_to_string(TransactionData *transaction_data)
    {
        std::string result = "";
        char temp[64];
        result += "chainId: " + std::to_string(to_uint64_t(transaction_data->chainId, transaction_data->chainIdLen)) + "\n";

        result += "nonce: " + std::to_string(to_uint64_t(transaction_data->nonce, transaction_data->nonceLen)) + "\n";

        bignum256 maxPriorityFee = to_bignum256(
            transaction_data->maxPriorityFeePerGas,
            transaction_data->maxPriorityFeePerGasLen);
        bn_format(
            &maxPriorityFee,
            "", "", 0, 0, false, temp, sizeof(temp));
        result += "maxPriorityFeePerGas: " + std::string(temp) + "\n";

        bignum256 maxFee = to_bignum256(
            transaction_data->maxFeePerGas,
            transaction_data->maxFeePerGasLen);
        bn_format(
            &maxFee,
            "", "", 0, 0, false, temp, sizeof(temp));
        result += "maxFeePerGas: " + std::string(temp) + "\n";

        bignum256 gasLimit = to_bignum256(
            transaction_data->gasLimit,
            transaction_data->gasLimitLen);
        bn_format(
            &gasLimit,
            "", "", 0, 0, false, temp, sizeof(temp));
        result += "gasLimit: " + std::string(temp) + "\n";

        result += "to: 0x" + to_hex_string(transaction_data->to, transaction_data->toLen) + "\n";

        bignum256 value = to_bignum256(
            transaction_data->value,
            transaction_data->valueLen);
        bn_format(
            &value,
            "", "", 0, 0, false, temp, sizeof(temp));
        result += "value: " + std::string(temp) + "\n";

        result += "data: 0x" + to_hex_string(transaction_data->data, transaction_data->dataLen) + "\n";

        return strdup(result.c_str());
    }
}