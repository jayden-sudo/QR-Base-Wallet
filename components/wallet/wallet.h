#ifndef WALLET_H
#define WALLET_H

/*********************
 *      INCLUDES
 *********************/

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif
    /*********************
     *      DEFINES
     *********************/

    /**********************
     *      TYPEDEFS
     **********************/
    typedef void *Wallet;
    typedef struct __attribute__((aligned(4)))
    {
        uint8_t public_key[33];
        uint8_t chain_code[32];
        uint8_t fingerprint[4];
    } PublicKeyFingerprint;

    /**********************
     * GLOBAL PROTOTYPES
     **********************/
    Wallet wallet_init_from_mnemonic(const char *mnemonic);
    Wallet wallet_init_from_xprv(const char *xprv);
    void wallet_free(Wallet wallet);

    char *wallet_root_private_key(Wallet wallet);
    void wallet_eth_key_fingerprint(Wallet wallet, PublicKeyFingerprint *fingerprint);
    Wallet wallet_derive(Wallet wallet, const char *path);
    Wallet wallet_derive_btc(Wallet wallet, unsigned int index);
    Wallet wallet_derive_eth(Wallet wallet, unsigned int index);
    void wallet_get_btc_address_segwit(Wallet wallet, char address[43]);
    void wallet_get_eth_address(Wallet wallet, char address[43]);
    void wallet_eth_sign(Wallet wallet, const uint8_t hash[32], uint8_t signature[65]);
    void wallet_eth_sign_serialized_data(Wallet wallet, uint8_t *serialized_data, size_t serialized_data_len, uint8_t signature[65]);
    void wallet_eth_sign_personal_message(Wallet wallet, char *message, uint8_t signature[65]);
    void wallet_keccak256_eip191(char *data, unsigned char digest[33]);
    void wallet_bin_to_hex_string(const uint8_t *bin, size_t bin_len, char **hex_string);

#ifdef __cplusplus
}
#endif

#endif /* WALLET_H */