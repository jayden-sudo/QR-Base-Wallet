#include <Arduino.h>
#include "wallet.h"
#include "aes.h"
#include <EEPROM.h>
#include <utility/trezor/bip39.h>
#include "qrcode_protocol.h"
#include "transaction_factory.h"

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
  Serial.begin(9600);
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

      String qrCode = generate_metamask_crypto_hdkey(wallet);
      Serial.println(qrCode);
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
