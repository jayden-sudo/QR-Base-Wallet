#include "transaction_factory.h"
#include <string.h>
#include <stdio.h>

#define TRANSACTION_TYPE_LEGACY 0
#define TRANSACTION_TYPE_ACCESS_LIST_EIP2930 1
#define TRANSACTION_TYPE_FEE_MARKET_EIP1559 2
#define TRANSACTION_TYPE_BLOB_EIP4844 3
#define TRANSACTION_TYPE_EOA_CODE_EIP7702 4

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

TransactionFactory::TransactionFactory(int error)
{
    this->error = error;
}

TransactionFactory::TransactionFactory(
    uint8_t *chainId,
    size_t chainIdLen,
    uint8_t *nonce,
    size_t nonceLen,
    uint8_t *maxPriorityFeePerGas,
    size_t maxPriorityFeePerGasLen,
    uint8_t *maxFeePerGas,
    size_t maxFeePerGasLen,
    uint8_t *gasLimit,
    size_t gasLimitLen,
    uint8_t *to,
    size_t toLen,
    uint8_t *value,
    size_t valueLen,
    uint8_t *data,
    size_t dataLen)
{

    BUFFER_TO_BIGNUMBER(chainId, chainIdLen, &this->chainId);
    char nonceStr[10] = {0};
    BUFFER_TO_HEX(nonceStr, nonce, nonceLen);
    this->nonce = strtol(nonceStr, NULL, 16);
    BUFFER_TO_BIGNUMBER(maxPriorityFeePerGas, maxPriorityFeePerGasLen, &this->maxPriorityFeePerGas);
    BUFFER_TO_BIGNUMBER(maxFeePerGas, maxFeePerGasLen, &this->maxFeePerGas);
    BUFFER_TO_BIGNUMBER(gasLimit, gasLimitLen, &this->gasLimit);
    char toStr[41] = {0};
    BUFFER_TO_HEX(toStr, to, toLen);
    this->to = String("0x") + String(toStr);
    BUFFER_TO_BIGNUMBER(value, valueLen, &this->value);
    char *dataStr = new char[dataLen * 2 + 1];
    dataStr[dataLen * 2] = '\0';
    BUFFER_TO_HEX(dataStr, data, dataLen);
    this->data = String("0x") + String(dataStr);
    delete[] dataStr;
}
TransactionFactory TransactionFactory::fromSerializedData(uint8_t *input, const size_t input_size)
{
    if (input[0] <= 0x7f)
    {
        // Determine the type.
        switch (input[0])
        {
        case TRANSACTION_TYPE_ACCESS_LIST_EIP2930:
        {
            return TransactionFactory(1);
        }
        case TRANSACTION_TYPE_FEE_MARKET_EIP1559:
        {
            // remove the first byte
            DecodeResult re = rlp_decode(input + 1, input_size - 1);
            if (re.error != 0)
            {
                return TransactionFactory(1);
            }

            if (re.item.is_list != 1 || re.item.data.list.count != 9)
            {
                return TransactionFactory(1);
            }
#define DATA_INPUT(index)                \
    re.item.data.list.items[index].data, \
        re.item.data.list.items[index].length

            TransactionFactory _transactionFactory = TransactionFactory(
                DATA_INPUT(0),
                DATA_INPUT(1),
                DATA_INPUT(2),
                DATA_INPUT(3),
                DATA_INPUT(4),
                DATA_INPUT(5),
                DATA_INPUT(6),
                DATA_INPUT(7));
            free_decode_result(&re);
            return _transactionFactory;
        }
        case TRANSACTION_TYPE_BLOB_EIP4844:
        {
            return TransactionFactory(1);
        }

        case TRANSACTION_TYPE_EOA_CODE_EIP7702:
        {
            return TransactionFactory(1);
        }
        default:
        {
            return TransactionFactory(1);
        }
        }
    }
    else
    {
        // TransactionType_Legacy
        return TransactionFactory(1);
    }
}

String TransactionFactory::toString()
{

    String result = "";

    char temp[64];
    size_t tempLen;

    tempLen = bn_format(&this->chainId, "", "", 0, 0, false, temp, sizeof(temp));
    result += "chainId: " + String(temp) + "\n";

    result += "nonce: " + String(this->nonce) + "\n";

    tempLen = bn_format(&this->maxPriorityFeePerGas, "", "", 0, 0, false, temp, sizeof(temp));
    result += "maxPriorityFeePerGas: " + String(temp) + "\n";

    tempLen = bn_format(&this->maxFeePerGas, "", "", 0, 0, false, temp, sizeof(temp));
    result += "maxFeePerGas: " + String(temp) + "\n";

    tempLen = bn_format(&this->gasLimit, "", "", 0, 0, false, temp, sizeof(temp));
    result += "gasLimit: " + String(temp) + "\n";

    result += "to: " + this->to + "\n";

    tempLen = bn_format(&this->value, "", "", 0, 0, false, temp, sizeof(temp));
    result += "value: " + String(temp) + "\n";

    result += "data: " + this->data + "\n";

    return result;
}