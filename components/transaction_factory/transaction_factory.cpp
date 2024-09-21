#include "transaction_factory.hpp"
#include <Conversion.h>

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
    printf("TransactionFactory failed with error:");
    printf("%d", error);
    this->error = error;
}

TransactionFactory::TransactionFactory(
    uint8_t *rlp_encoded,
    size_t rlp_encoded_len,

    uint8_t transactionType,
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
    size_t dataLen,
    uint8_t *accessList,
    size_t accessListLen)
{
    this->rlp_encoded = new uint8_t[rlp_encoded_len];
    memcpy(this->rlp_encoded, rlp_encoded, rlp_encoded_len);
    this->rlp_encoded_len = rlp_encoded_len;

    this->transactionType = transactionType;
    this->chainId = chainId;
    this->chainIdLen = chainIdLen;
    this->nonce = nonce;
    this->nonceLen = nonceLen;
    this->maxPriorityFeePerGas = maxPriorityFeePerGas;
    this->maxPriorityFeePerGasLen = maxPriorityFeePerGasLen;
    this->maxFeePerGas = maxFeePerGas;
    this->maxFeePerGasLen = maxFeePerGasLen;
    this->gasLimit = gasLimit;
    this->gasLimitLen = gasLimitLen;
    this->to = to;
    this->toLen = toLen;
    this->value = value;
    this->valueLen = valueLen;
    this->data = data;
    this->dataLen = dataLen;
    this->accessList = accessList;
    this->accessListLen = accessListLen;
}

#define TRANSACTION_ERROR(code)                                     \
    TransactionFactory *transaction = new TransactionFactory(code); \
    return transaction;

TransactionFactory *TransactionFactory::fromSerializedData(uint8_t *input, const size_t input_size)
{
    if (input[0] <= 0x7f)
    {
        // Ethereum Transaction Type
        switch (input[0])
        {
        case TRANSACTION_TYPE_ACCESS_LIST_EIP2930:
        {
            // #TODO
            // not implemented
            TRANSACTION_ERROR(1);
        }
        case TRANSACTION_TYPE_FEE_MARKET_EIP1559:
        {
            uint8_t *chainId;
            size_t chainIdLen;
            uint8_t *nonce;
            size_t nonceLen;
            uint8_t *maxPriorityFeePerGas;
            size_t maxPriorityFeePerGasLen;
            uint8_t *maxFeePerGas;
            size_t maxFeePerGasLen;
            uint8_t *gasLimit;
            size_t gasLimitLen;
            uint8_t *to;
            size_t toLen;
            uint8_t *value;
            size_t valueLen;
            uint8_t *data;
            size_t dataLen;
            uint8_t *accessList;
            size_t accessListLen;

            uint8_t *rlp_encoded_ptr = input;
            size_t rlp_encoded_len = input_size;

            rlp_encoded_ptr++;
            rlp_encoded_len--;

            struct RLP_ITEM item;

            rlp_decode(rlp_encoded_ptr, rlp_encoded_len, &item);
            if (item.type != RLP_ITEM_LIST)
            {
                TRANSACTION_ERROR(2);
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
                    TRANSACTION_ERROR(3);
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
                    TRANSACTION_ERROR(4);
                }
            }
            TransactionFactory *transactionFactory = new TransactionFactory(
                input,
                input_size,
                TRANSACTION_TYPE_FEE_MARKET_EIP1559,
                chainId,
                chainIdLen,
                nonce,
                nonceLen,
                maxPriorityFeePerGas,
                maxPriorityFeePerGasLen,
                maxFeePerGas,
                maxFeePerGasLen,
                gasLimit,
                gasLimitLen,
                to,
                toLen,
                value,
                valueLen,
                data,
                dataLen,
                accessList,
                accessListLen);
            return transactionFactory;
        }
        case TRANSACTION_TYPE_BLOB_EIP4844:
        {
            // #TODO
            // not implemented
            TRANSACTION_ERROR(1);
        }

        case TRANSACTION_TYPE_EOA_CODE_EIP7702:
        {
            // #TODO
            // not implemented
            TRANSACTION_ERROR(1);
        }
        default:
        {
            // #TODO
            // not implemented
            TRANSACTION_ERROR(1);
        }
        }
    }
    else
    {
        // TransactionType_Legacy
        // #TODO
        // not implemented
        TRANSACTION_ERROR(1);
    }
}

uint64_t TransactionFactory::to_uint64_t(uint8_t *input, size_t input_size)
{
    bignum256 _data = TransactionFactory::to_bignum256(input, input_size);
    return bn_write_uint64(&_data);
}
bignum256 TransactionFactory::to_bignum256(uint8_t *input, size_t input_size)
{
    bignum256 _data;
    BUFFER_TO_BIGNUMBER(input, input_size, &_data);
    return _data;
}
std::string TransactionFactory::to_hex_string(uint8_t *input, size_t input_size)
{
    std::string _data = toHex(input, input_size);
    return _data;
}

std::string TransactionFactory::toString()
{

    std::string result = "";

    char temp[64];

    result += "chainId: " + std::to_string(TransactionFactory::to_uint64_t(this->chainId, this->chainIdLen)) + "\n";

    result += "nonce: " + std::to_string(TransactionFactory::to_uint64_t(this->nonce, this->nonceLen)) + "\n";

    bignum256 maxPriorityFee = TransactionFactory::to_bignum256(
        this->maxPriorityFeePerGas,
        this->maxPriorityFeePerGasLen);
    bn_format(
        &maxPriorityFee,
        "", "", 0, 0, false, temp, sizeof(temp));
    result += "maxPriorityFeePerGas: " + std::string(temp) + "\n";

    bignum256 maxFee = TransactionFactory::to_bignum256(
        this->maxFeePerGas,
        this->maxFeePerGasLen);
    bn_format(
        &maxFee,
        "", "", 0, 0, false, temp, sizeof(temp));
    result += "maxFeePerGas: " + std::string(temp) + "\n";

    bignum256 gasLimit = TransactionFactory::to_bignum256(
        this->gasLimit,
        this->gasLimitLen);
    bn_format(
        &gasLimit,
        "", "", 0, 0, false, temp, sizeof(temp));
    result += "gasLimit: " + std::string(temp) + "\n";

    result += "to: 0x" + TransactionFactory::to_hex_string(this->to, this->toLen) + "\n";

    bignum256 value = TransactionFactory::to_bignum256(
        this->value,
        this->valueLen);
    bn_format(
        &value,
        "", "", 0, 0, false, temp, sizeof(temp));
    result += "value: " + std::string(temp) + "\n";

    result += "data: 0x" + TransactionFactory::to_hex_string(this->data, this->dataLen) + "\n";

    return result;
}

TransactionFactory::~TransactionFactory()
{
    if (this->rlp_encoded != nullptr)
    {
        delete[] this->rlp_encoded;
    }
}