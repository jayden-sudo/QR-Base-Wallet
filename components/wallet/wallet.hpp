#pragma once

/*
  #define USE_KECCAK 1
 */

#include <cstdint>
#include <cstddef>
#include <string>
#include <Bitcoin.h>
#include <Hash.h>
#include <utility/trezor/sha3.h>
#include <utility/trezor/secp256k1.h>
#include <utility/trezor/ecdsa.h>
#include <transaction_factory.hpp>

class HDPrivateKey; // Forward declaration

struct PublicKeyFingerprint
{
  uint8_t public_key[33] = {0};
  uint8_t chain_code[32] = {0};
  uint8_t fingerprint[4] = {0};
};

class Wallet
{
private:
  HDPrivateKey hd;

public:
  Wallet(const std::string mnemonic);
  Wallet(const char *xprvArr);
  ~Wallet();

  std::string root_private_key();
  PublicKeyFingerprint public_eth_key_fingerprint();

  HDPrivateKey derive(std::string path);

  HDPrivateKey derive_btc(unsigned int index);
  static std::string get_btc_address_segwit(HDPrivateKey account);

  HDPrivateKey derive_eth(unsigned int index);
  static std::string get_eth_address(HDPrivateKey account);
  static std::string eth_sign(HDPrivateKey account, const uint8_t hash[32]);
  static void eth_sign_serialized_data(HDPrivateKey account, uint8_t *serialized_data, size_t serialized_data_len, uint8_t signature[65]);
  static int keccak_256_eip191(std::string data, unsigned char *digest);
};