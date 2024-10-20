#ifndef QRCODE_PROTOCOL_H
#define QRCODE_PROTOCOL_H

/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "wallet.h"
#include <stdbool.h>

/*********************
 *      DEFINES
 *********************/
#define METAMASK_ETH_SIGN_REQUEST "eth-sign-request"
#define METAMASK_CRYPTO_HDKEY "crypto-hdkey"
#define METAMASK_ETH_SIGNATURE "eth-signature"

#define KEY_DATA_TYPE_SIGN_TRANSACTION 1
#define KEY_DATA_TYPE_SIGN_TYPED_DATA 2
#define KEY_DATA_TYPE_SIGN_PERSONAL_MESSAGE 3
#define KEY_DATA_TYPE_SIGN_TYPED_TRANSACTION 4

#ifdef __cplusplus
extern "C"
{
#endif

    /**********************
     *      TYPEDEFS
     **********************/
    typedef void *UR;
    typedef void *URDecoder;

    typedef struct __attribute__((aligned(4))) _MetamaskSignRequest
    {
        char *uuid_base64url;
        char *sign_data_base64url;
        uint32_t data_type;
        uint64_t chain_id;
        char *derivation_path;
        char *address;
    } MetamaskSignRequest;

    typedef enum
    {
        Invalid = 0,
        SinglePart = 1,
        MultiPart = 2
    } URType;

    typedef struct __attribute__((aligned(4))) _qrcode_protocol_bc_ur_data_t
    {
        URType ur_type;
        UR ur;
        URDecoder ur_decoder;
    } qrcode_protocol_bc_ur_data_t;

    /**********************
     * GLOBAL PROTOTYPES
     **********************/
    void generate_metamask_crypto_hdkey(Wallet *wallet, char **output);
    void generate_metamask_eth_signature(uint8_t *uuid_str, uint8_t signature[65], char **output);
    int decode_metamask_sign_request(UR ur, MetamaskSignRequest *request);
    void free_metamask_sign_request(MetamaskSignRequest *request);

    URType ur_type(const char *url);
    void qrcode_protocol_bc_ur_init(qrcode_protocol_bc_ur_data_t *data);
    void qrcode_protocol_bc_ur_free(qrcode_protocol_bc_ur_data_t *data);
    bool qrcode_protocol_bc_ur_receive(qrcode_protocol_bc_ur_data_t *data, const char *receiveStr);
    bool qrcode_protocol_bc_ur_is_complete(qrcode_protocol_bc_ur_data_t *data);
    bool qrcode_protocol_bc_ur_is_success(qrcode_protocol_bc_ur_data_t *data);
    const char *qrcode_protocol_bc_ur_type(qrcode_protocol_bc_ur_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* QRCODE_PROTOCOL_H */
