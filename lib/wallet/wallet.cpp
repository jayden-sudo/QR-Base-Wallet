#include <wallet.h>

#define MAX_PATH_LEN 32
#define ETHEREUM_SIG_PREFIX "\u0019Ethereum Signed Message:\n"
#define ETH_DERIVATION_PATH "m/44'/60'/0'/"
#define BTC_DERIVATION_PATH "m/84'/0'/0'/"

Wallet::Wallet(const String mnemonic)
{
    this->hd = HDPrivateKey{mnemonic, ""};
}

Wallet::Wallet(const char *xprv_arr)
{
    this->hd = HDPrivateKey{xprv_arr};
}

Wallet::~Wallet()
{
}

String Wallet::root_private_key()
{
    return this->hd.xprv();
}

PublicKeyFingerprint Wallet::public_eth_key_fingerprint()
{
    HDPrivateKey account = this->hd.derive(ETH_DERIVATION_PATH);
    PublicKeyFingerprint publicKeyFingerprint;
    account.xpub().sec(publicKeyFingerprint.public_key, 33);
    memcpy(publicKeyFingerprint.chain_code, account.xpub().chainCode, 32);
    account.xpub().fingerprint(publicKeyFingerprint.fingerprint);
    return publicKeyFingerprint;
}

HDPrivateKey Wallet::derive_btc(unsigned int index)
{
    char *derive_path = new char[64];
    snprintf(derive_path, 64, "%s%d/%d/", BTC_DERIVATION_PATH, 0, index);
    HDPrivateKey account = this->hd.derive(derive_path);
    delete[] derive_path;
    return account;
}

String Wallet::get_btc_address_segwit(HDPrivateKey account)
{
    return account.segwitAddress();
}

HDPrivateKey Wallet::derive_eth(unsigned int index)
{
    char *derive_path = new char[64];
    snprintf(derive_path, 64, "%s%d/%d/", ETH_DERIVATION_PATH, 0, index);
    HDPrivateKey account = this->hd.derive(derive_path);
    delete[] derive_path;
    return account;
}

String Wallet::get_eth_address(HDPrivateKey account)
{
    uint8_t xy[64] = {0};
    byte eth_address[64] = {0};

    memcpy(xy, account.publicKey().point, 64);
    keccak_256(xy, 64, eth_address);
    return "0x" + toHex(eth_address + 12, 20);
}

String Wallet::eth_sign(HDPrivateKey account, const uint8_t hash[32])
{
    Signature sig = account.sign(hash);
    uint8_t signature[65] = {0};

    sig.index += 27;
    sig.bin(signature, 65);
    return toHex(signature, 65);
}

int Wallet::keccak_256_eip191(String data, unsigned char *digest)
{
    String prefix = ETHEREUM_SIG_PREFIX;
    const String data_len = String(data.length());
    const String data_to_hash = prefix + data_len + data;

    keccak_256((const unsigned char *)data_to_hash.c_str(), data_to_hash.length(), digest);
    return 32;
}