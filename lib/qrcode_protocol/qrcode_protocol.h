#pragma once

#include <stdint.h>
#include <WString.h>
#include <wallet.h>

#define METAMASK_ETH_SIGN_REQUEST "eth-sign-request"
#define METAMASK_CRYPTO_HDKEY "crypto-hdkey"
#define METAMASK_ETH_SIGNATURE "eth-signature"

struct MetamaskEthSignRequest
{
    String uuid_base64url;
    String sign_data_base64url;
    uint32_t data_type;
    uint64_t chain_id;
    String derivation_path;
    String address;
};

int decode_url(const String url, String *type, String *payload);
String encode_url(const String type, const String payload);

String generate_metamask_crypto_hdkey(Wallet *wallet);

String generate_metamask_eth_signature(uint8_t *uuid, size_t uuid_len, uint8_t signature[65]);

int decode_metamask_eth_sign_request(String qrcode, MetamaskEthSignRequest *request);