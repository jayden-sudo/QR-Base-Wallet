#pragma once

#include <rlp.h>
#include <WString.h>
#include <utility/trezor/bignum.h>

class TransactionFactory
{
private:
    TransactionFactory(int error);
    TransactionFactory(
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
        size_t dataLen);

public:
    int error = 0;
    bignum256 chainId;
    uint32_t nonce;
    bignum256 maxPriorityFeePerGas;
    bignum256 maxFeePerGas;
    bignum256 gasLimit;
    String to;
    bignum256 value;
    String data;
    static TransactionFactory fromSerializedData(uint8_t *input, size_t input_size);
    String toString();
};