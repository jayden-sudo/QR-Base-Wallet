#include <Arduino.h>
#include "wallet.h"
#include "aes.h"
#include <EEPROM.h>
#include <utility/trezor/bip39.h>
#include "qrcode_protocol.h"
#include "transaction_factory.h"
#include <base64url.h>
#include <bc-ur.h>
#include <qrcodegen.h>

/*
  /Users/<user>/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32/qout_qspi/include/sdkconfig.h
 */

#define MAX_INCORRECT_PIN_ATTEMPTS 3
#define MIN_PIN_LENGTH 3
#define MAX_PIN_LENGTH 10
#define PRIVATE_KEY_SIZE AES_BLOCK_SIZE * 8

struct WalletData
{
  bool initialized = false;
  uint8_t incorrectPinCount = 0;
  unsigned char privateKey[PRIVATE_KEY_SIZE] = {0};
};

Wallet *wallet = nullptr;

String inputMnemonic();
String inputPin();
void initializeWallet(WalletData &walletData);
bool unlockWallet(WalletData &walletData);
void eraseWalletData(WalletData &walletData);

void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
    ; // wait for serial port to open
  }

  Serial.println("Setup started");

  WalletData walletData;
  EEPROM.begin(sizeof(WalletData));
  EEPROM.get(0, walletData);

  if (!walletData.initialized)
  {
    initializeWallet(walletData);
  }
  else
  {
    if (unlockWallet(walletData))
    {
      Serial.println("Wallet unlocked successfully");
    }
    else
    {
      Serial.println("Failed to unlock wallet");
      return;
    }
  }

  Serial.println("Setup completed");
}

bool once_after_unlock = false;
bool on_process = false;

void loop()
{
  if (wallet != nullptr)
  {
    if (once_after_unlock == false)
    {
      once_after_unlock = true;

      Serial.print("ETH Address #0: ");
      Serial.println(Wallet::get_eth_address(wallet->derive_eth(0)));
      Serial.print("ETH Address #1: ");
      Serial.println(Wallet::get_eth_address(wallet->derive_eth(1)));
      Serial.print("ETH Address #2: ");
      Serial.println(Wallet::get_eth_address(wallet->derive_eth(2)));

      Serial.println("Metamask Connect QR code:");
      String qrCode = generate_metamask_crypto_hdkey(wallet);
      Serial.println(qrCode);
      Serial.println("=======================");
    }
    if (on_process == false)
    {
      on_process = true;
      {
        Serial.println("Input QR code:");
        while (Serial.available() == 0)
        {
          ;
        }
        String qrcode_input = Serial.readString();
        // qrcode_input = "UR:ETH-SIGN-REQUEST/OLADTPDAGDHEWFEMLOBSMNGSFXLYASWKPDDRMEGTQDAOHDIMAOYAIOLSPKENOSLALRHKISDLAELRRPAHCTBSLFLYESMWNEVOESHLIOINKSENQZNEGMFTGYFPOSYAZMTIATIMLAROFNBYCPEOFYGOIYKTLONLAEBYCPEOFYGOIYKTLONLAEBYCPEOFYGOIYKTLONLAEBYCPEOFYGOIYKTLONLAEBYCPEOFYGOIYKTLONLAEBYCPEOFYGOIYKTLONLAERTAXAAAACYAEPKENOSAHTAADDYOEADLECSDWYKCSFNYKAEYKAEWKAEWKAOCYWLDAQZPRAMGHNEVOESHLIOINKSENQZNEGMFTGYFPOSYAZMTIATIMCNMYJTRE";
        // qrcode_input = "UR:ETH-SIGN-REQUEST/OLADTPDAGDBKAABWRDVOHHGEBBNEWKCAWLRPLERSKTAOHDECAOWFLSPKENOSLALRHKISDLAELPBNRODYEMDYLFGMAYMWMYIATSUTIMFHHKETHSJTWTHNCMRKWZHPTBAOEOBZLOADIAFEKSHLLEAEAELARTAXAAAACYAEPKENOSAHTAADDYOEADLECSDWYKCSFNYKAEYKAEWKAEWKAOCYWLDAQZPRAMGHNEVOESHLIOINKSENQZNEGMFTGYFPOSYAZMTIATIMFGVLDWFW";
        qrcode_input.trim();
        {
          String _type;
          String _payload;
          if (decode_url(qrcode_input, &_type, &_payload) == 0)
          {
            if (_type == METAMASK_ETH_SIGN_REQUEST)
            {
              MetamaskEthSignRequest request;
              decode_metamask_eth_sign_request(_payload, &request);

              size_t sign_data_max_len = request.sign_data_base64url.length();
              uint8_t *sign_data = new uint8_t[sign_data_max_len];
              size_t sign_data_len = decode_base64url(request.sign_data_base64url, sign_data, sign_data_max_len);
              String sign_data_hex = toHex(sign_data, sign_data_len);
              Serial.println("sign_data_hex:");
              Serial.println(sign_data_hex);
              Serial.println(request.data_type);
              Serial.println(request.chain_id);
              Serial.println(request.derivation_path);
              Serial.println(request.address);
              Serial.println("Creating transaction factory...");
              TransactionFactory *transaction = TransactionFactory::fromSerializedData(sign_data, sign_data_len);
              do
              {
                if (transaction->error == 0)
                {

                  HDPrivateKey account = wallet->derive(request.derivation_path);
                  if (Wallet::get_eth_address(account) != request.address)
                  {
                    Serial.println("Invalid address");
                  }
                  else
                  {
                    Serial.println("Signing transaction...");
                    uint8_t signature[65];
                    Wallet::eth_sign_serialized_data(account, sign_data, sign_data_len, signature);
                    size_t uuid_max_len = request.uuid_base64url.length();
                    uint8_t *uuid = new uint8_t[uuid_max_len];
                    size_t uuid_len = decode_base64url(request.uuid_base64url, uuid, uuid_max_len);
                    String qr_code = generate_metamask_eth_signature(uuid, uuid_len, signature);
                    delete[] uuid;
                    Serial.println("Signature QR code:");
                    Serial.println(qr_code);
                    Serial.println("QR code sent");
                    Serial.println("=======================");
                    Serial.println("");
                    Serial.println("=======================");
                  }
                }
              } while (0);
              delete[] sign_data;
              delete transaction;
            }
            else
            {
              Serial.print("Unsupported QR code type: ");
              Serial.println(_type);
            }
          }
          else
          {

            Serial.println("Invalid QR code");
          }
        }
      }
      on_process = false;
    }
  }
}

String inputMnemonic()
{
  while (true)
  {
    Serial.println("Input mnemonic:");
    while (Serial.available() == 0)
    {
    }
    String mnemonic = Serial.readString();
    mnemonic.trim();

    Serial.print("Mnemonic: [");
    Serial.print(mnemonic);
    Serial.println("]");

    if (mnemonic_check(mnemonic.c_str()) != 0)
    {
      return mnemonic;
    }
    Serial.println("Invalid mnemonic, try again.");
  }
}

String inputPin()
{
  while (true)
  {
    Serial.println("Input PIN:");
    while (Serial.available() == 0)
    {
    }
    String pin = Serial.readString();
    pin.trim();

    if (pin.length() < MIN_PIN_LENGTH || pin.length() > MAX_PIN_LENGTH)
    {
      Serial.println("Invalid PIN length, try again.");
      continue;
    }

    // only digits
    for (char c : pin)
    {
      if (c < '0' || c > '9')
      {
        Serial.println("Invalid PIN, try again.");
        continue;
      }
    }

    byte hash[64] = {0};
    int hashLen = sha256(pin + " freedom wallet pin", hash);
    return toHex(hash, hashLen);
  }
}

void initializeWallet(WalletData &walletData)
{
  if (wallet != nullptr)
  {
    // can't initialize wallet twice
    return;
  }
  String mnemonic = inputMnemonic();
  Wallet *_wallet = new Wallet(mnemonic);
  String rootPrivateKey = _wallet->root_private_key();
  String pin = inputPin();
  AES_encrypt(reinterpret_cast<const unsigned char *>(pin.c_str()),
              reinterpret_cast<const unsigned char *>(rootPrivateKey.c_str()),
              PRIVATE_KEY_SIZE, walletData.privateKey);
  EEPROM.put(0, walletData);
  EEPROM.commit();
  wallet = _wallet;
  walletData.initialized = true;
  Serial.println("Wallet initialized");
}

bool unlockWallet(WalletData &walletData)
{
  while (true)
  {
    String pin = inputPin();
    char rootPrivateKey[PRIVATE_KEY_SIZE] = {0};
    AES_decrypt(reinterpret_cast<const unsigned char *>(pin.c_str()),
                walletData.privateKey, PRIVATE_KEY_SIZE,
                reinterpret_cast<unsigned char *>(rootPrivateKey));

    String rootPrivateKeyStr = String(rootPrivateKey);
    if (rootPrivateKeyStr.startsWith("xprv"))
    {
      if (walletData.incorrectPinCount > 0)
      {
        walletData.incorrectPinCount = 0;
        EEPROM.put(0, walletData);
        EEPROM.commit();
      }
      wallet = new Wallet(rootPrivateKey);
      Serial.println("Wallet unlocked");
      return true;
    }

    walletData.incorrectPinCount++;
    if (walletData.incorrectPinCount >= MAX_INCORRECT_PIN_ATTEMPTS)
    {
      Serial.println("Incorrect PIN count >= 3, erasing wallet data.");
      eraseWalletData(walletData);
      return false;
    }

    Serial.println("Incorrect PIN, try again.");
    EEPROM.put(0, walletData);
    EEPROM.commit();
  }
}

void eraseWalletData(WalletData &walletData)
{
  walletData.initialized = false;
  walletData.incorrectPinCount = 0;
  memset(walletData.privateKey, 0, sizeof(walletData.privateKey));
  EEPROM.put(0, walletData);
  EEPROM.commit();
}
