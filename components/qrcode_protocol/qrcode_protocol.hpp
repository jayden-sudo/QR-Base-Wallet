#pragma once

#include <string>
#include "wallet.hpp"
#include "bc-ur.hpp"

#define METAMASK_ETH_SIGN_REQUEST "eth-sign-request"
#define METAMASK_CRYPTO_HDKEY "crypto-hdkey"
#define METAMASK_ETH_SIGNATURE "eth-signature"

#define KEY_DATA_TYPE_SIGN_TYPED_DATA 3
#define KEY_DATA_TYPE_SIGN_TRANSACTION 4

struct MetamaskEthSignRequest
{
    std::string uuid_base64url;
    std::string sign_data_base64url;
    uint32_t data_type;
    uint64_t chain_id;
    std::string derivation_path;
    std::string address;
};

enum class URType
{
    Invalid = 0,
    SinglePart = 1,
    MultiPart = 2
};

URType ur_type(const std::string &url);

std::string generate_metamask_crypto_hdkey(Wallet *wallet);

std::string generate_metamask_eth_signature(uint8_t *uuid, size_t uuid_len, uint8_t signature[65]);

int decode_metamask_eth_sign_request(ur::UR *ur, MetamaskEthSignRequest *request);
