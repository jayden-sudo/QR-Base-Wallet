#ifndef TRANSACTION_FACTORY_H
#define TRANSACTION_FACTORY_H

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stdlib.h>

/*********************
 *      DEFINES
 *********************/
#define TRANSACTION_TYPE_LEGACY 0
#define TRANSACTION_TYPE_ACCESS_LIST_EIP2930 1
#define TRANSACTION_TYPE_FEE_MARKET_EIP1559 2
#define TRANSACTION_TYPE_BLOB_EIP4844 3
#define TRANSACTION_TYPE_EOA_CODE_EIP7702 4

#ifdef __cplusplus
extern "C"
{
#endif

    /**********************
     *      TYPEDEFS
     **********************/

    typedef struct __attribute__((aligned(4)))
    {
        /* error=0: success, error!=0: failed */
        int error;
        uint8_t *rlp_encoded;
        size_t rlp_encoded_len;
        uint8_t transactionType;
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
    } TransactionData;

    /**********************
     * GLOBAL PROTOTYPES
     **********************/
    void transaction_factory_init(TransactionData *transaction_data, const uint8_t *sign_data, size_t sign_data_len);
    void transaction_factory_free(TransactionData *transaction_data);
    char *transaction_factory_to_string(TransactionData *transaction_data);

#ifdef __cplusplus
}
#endif

#endif /* TRANSACTION_FACTORY_H */