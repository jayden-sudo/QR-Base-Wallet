#pragma once

#include <string>
#include <wallet.hpp>

#define METAMASK_ETH_SIGN_REQUEST "eth-sign-request"
#define METAMASK_CRYPTO_HDKEY "crypto-hdkey"
#define METAMASK_ETH_SIGNATURE "eth-signature"

struct MetamaskEthSignRequest
{
    std::string uuid_base64url;
    std::string sign_data_base64url;
    uint32_t data_type;
    uint64_t chain_id;
    std::string derivation_path;
    std::string address;
};

int decode_url(const std::string url, std::string *type, std::string *payload);
std::string encode_url(const std::string type, const std::string payload);

std::string generate_metamask_crypto_hdkey(Wallet *wallet);

std::string generate_metamask_eth_signature(uint8_t *uuid, size_t uuid_len, uint8_t signature[65]);

int decode_metamask_eth_sign_request(std::string qrcode, MetamaskEthSignRequest *request);