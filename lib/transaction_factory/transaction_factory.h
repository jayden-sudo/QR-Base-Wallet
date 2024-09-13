#pragma once

#include <rlp.h>
#include <WString.h>
#include <vector>
#include <utility/trezor/bignum.h>

#define TRANSACTION_TYPE_LEGACY 0
#define TRANSACTION_TYPE_ACCESS_LIST_EIP2930 1
#define TRANSACTION_TYPE_FEE_MARKET_EIP1559 2
#define TRANSACTION_TYPE_BLOB_EIP4844 3
#define TRANSACTION_TYPE_EOA_CODE_EIP7702 4

class TransactionFactory
{
private:
    TransactionFactory(int error);
    TransactionFactory(
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
        size_t accessListLen);

public:
    int error = 0;

    uint8_t transactionType;
    bignum256 chainId;
    uint32_t nonce;
    bignum256 maxPriorityFeePerGas;
    bignum256 maxFeePerGas;
    bignum256 gasLimit;
    String to;
    bignum256 value;
    String data;
    std::vector<String> accessList;
    static TransactionFactory fromSerializedData(uint8_t *input, size_t input_size);
    ~TransactionFactory();
    String toString();
};