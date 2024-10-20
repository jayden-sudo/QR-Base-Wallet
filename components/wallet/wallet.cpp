/*********************
 *      INCLUDES
 *********************/
#include <wallet.h>
#include "rlp.h"
#include <cstdint>
#include <cstddef>
#include <string>
#include <Bitcoin.h>
#include <Hash.h>
#include <utility/trezor/sha3.h>
#include <utility/trezor/secp256k1.h>
#include <utility/trezor/ecdsa.h>
#include <transaction_factory.h>
#include <memory>
#include <unordered_map>

/*********************
 *      DEFINES
 *********************/
#define MAX_PATH_LEN 32
#define ETHEREUM_SIG_PREFIX "\u0019Ethereum Signed Message:\n"
#define ETH_DERIVATION_PATH "m/44'/60'/0'/"
#define BTC_DERIVATION_PATH "m/84'/0'/0'/"

/**********************
 *  STATIC VARIABLES
 **********************/
static std::unordered_map<int, std::shared_ptr<HDPrivateKey>> shared_ptr_map;

extern "C"
{

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

    /**********************
     *   GLOBAL FUNCTIONS
     **********************/
    Wallet wallet_init_from_mnemonic(const char *mnemonic)
    {
        auto wallet = HDPrivateKey{mnemonic, ""};
        std::shared_ptr<HDPrivateKey> _wallet = std::make_shared<HDPrivateKey>(wallet);
        int ptr = (int)_wallet.get();
        shared_ptr_map[ptr] = _wallet;
        return (Wallet)ptr;
    }
    Wallet wallet_init_from_xprv(const char *xprv)
    {
        auto wallet = HDPrivateKey{xprv};
        std::shared_ptr<HDPrivateKey> _wallet = std::make_shared<HDPrivateKey>(wallet);
        int ptr = (int)_wallet.get();
        shared_ptr_map[ptr] = _wallet;
        return (Wallet)ptr;
    }
    void wallet_free(Wallet wallet)
    {
        int ptr = (int)wallet;
        if (shared_ptr_map.find(ptr) != shared_ptr_map.end())
        {
            shared_ptr_map.erase(ptr);
        }
    }

    char *wallet_root_private_key(Wallet wallet)
    {
        HDPrivateKey *_wallet = (HDPrivateKey *)wallet;
        auto str = _wallet->xprv();
        auto *cstr = new char[str.length() + 1];
        strcpy(cstr, str.c_str());
        return cstr;
    }
    void wallet_eth_key_fingerprint(Wallet wallet, PublicKeyFingerprint *fingerprint)
    {
        HDPrivateKey *_wallet = (HDPrivateKey *)wallet;
        HDPrivateKey account = _wallet->derive(ETH_DERIVATION_PATH);
        account.xpub().sec(fingerprint->public_key, 33);
        memcpy(fingerprint->chain_code, account.xpub().chainCode, 32);
        account.xpub().fingerprint(fingerprint->fingerprint);
    }
    Wallet wallet_derive(Wallet wallet, const char *path)
    {
        HDPrivateKey *_wallet = (HDPrivateKey *)wallet;
        auto derived = _wallet->derive(path);
        std::shared_ptr<HDPrivateKey> _derived = std::make_shared<HDPrivateKey>(derived);
        int ptr = (int)_derived.get();
        shared_ptr_map[ptr] = _derived;
        return (Wallet)ptr;
    }
    Wallet wallet_derive_btc(Wallet wallet, unsigned int index)
    {
        HDPrivateKey *_wallet = (HDPrivateKey *)wallet;
        char *derive_path = new char[64];
        snprintf(derive_path, 64, "%s%d/%d/", BTC_DERIVATION_PATH, 0, index);
        HDPrivateKey account = _wallet->derive(derive_path);
        delete[] derive_path;
        std::shared_ptr<HDPrivateKey> _derived = std::make_shared<HDPrivateKey>(account);
        int ptr = (int)_derived.get();
        shared_ptr_map[ptr] = _derived;
        return (Wallet)ptr;
    }

    void wallet_get_btc_address_segwit(Wallet wallet, char address[43])
    {
        HDPrivateKey *_wallet = (HDPrivateKey *)wallet;
        auto str = _wallet->segwitAddress();
        strcpy(address, str.c_str());
    }
    Wallet wallet_derive_eth(Wallet wallet, unsigned int index)
    {
        HDPrivateKey *_wallet = (HDPrivateKey *)wallet;
        char *derive_path = new char[64];
        snprintf(derive_path, 64, "%s%d/%d/", ETH_DERIVATION_PATH, 0, index);
        auto derived = _wallet->derive(derive_path);
        delete[] derive_path;
        std::shared_ptr<HDPrivateKey> _derived = std::make_shared<HDPrivateKey>(derived);
        int ptr = (int)_derived.get();
        shared_ptr_map[ptr] = _derived;
        return (Wallet)ptr;
    }
    void wallet_get_eth_address(Wallet wallet, char address[43])
    {
        HDPrivateKey *_wallet = (HDPrivateKey *)wallet;
        uint8_t xy[64] = {0};
        unsigned char eth_address[64] = {0};
        memcpy(xy, _wallet->publicKey().point, 64);
        keccak_256(xy, 64, eth_address);
        auto str = "0x" + toHex(eth_address + 12, 20);
        strcpy(address, str.c_str());
    }
    void wallet_eth_sign(Wallet wallet, const uint8_t hash[32], uint8_t signature[65])
    {
        HDPrivateKey *_wallet = (HDPrivateKey *)wallet;
        Signature sig = _wallet->sign(hash);
        sig.index += 27;
        sig.bin((uint8_t *)signature, 65);
    }
    void wallet_eth_sign_serialized_data(Wallet wallet, uint8_t *serialized_data, size_t serialized_data_len, uint8_t signature[65])
    {
        HDPrivateKey *_wallet = (HDPrivateKey *)wallet;
        unsigned char hash[32] = {0};
        // keccak256
        keccak_256(serialized_data, serialized_data_len, hash);
        // sign
        Signature sig = _wallet->sign(hash);
        sig.bin((uint8_t *)signature, 65);
    }
    void wallet_eth_sign_personal_message(Wallet wallet, char *message, uint8_t signature[65])
    {
        HDPrivateKey *_wallet = (HDPrivateKey *)wallet;
        unsigned char hash[33] = {0};
        wallet_keccak256_eip191(message, hash);
        Signature sig = _wallet->sign(hash);
        sig.bin((uint8_t *)signature, 65);
    }
    void wallet_keccak256_eip191(char *data, unsigned char digest[33])
    {
        std::string prefix = ETHEREUM_SIG_PREFIX;
        const std::string data_len = std::to_string(strlen(data));
        const std::string data_to_hash = prefix + data_len + data;
        keccak_256((const unsigned char *)data_to_hash.c_str(), data_to_hash.length(), digest);
    }

    void wallet_bin_to_hex_string(const uint8_t *bin, size_t bin_len, char **hex_string)
    {
        auto str = toHex(bin, bin_len);
        *hex_string = (char *)malloc(str.length() + 1);
        strcpy(*hex_string, str.c_str());
    }
}