#pragma once

/*
  #define USE_KECCAK 1
 */
#include <Bitcoin.h>
#include <Hash.h>
#include <utility/trezor/sha3.h>
#include <utility/trezor/secp256k1.h>
#include <utility/trezor/ecdsa.h>

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
  Wallet(const String mnemonic);
  Wallet(const char *xprvArr);
  ~Wallet();

  String root_private_key();
  PublicKeyFingerprint public_eth_key_fingerprint();

  HDPrivateKey derive_btc(unsigned int index);
  static String get_btc_address_segwit(HDPrivateKey account);

  HDPrivateKey derive_eth(unsigned int index);
  static String get_eth_address(HDPrivateKey account);
  static String eth_sign(HDPrivateKey account, const uint8_t hash[32]);
  static String eth_sign_serialized_data(HDPrivateKey account, const uint8_t *data, size_t dataLen);
  static int keccak_256_eip191(String data, unsigned char *digest);
};