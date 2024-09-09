#include <Arduino.h>
#include "wallet.h"
#include "aes/aes.h"
#include <EEPROM.h>
#include <utility/trezor/bip39.h>
#include <Hash.h>

#ifndef USE_KECCAK
#error "USE_KECCAK is not defined"
#endif
#ifndef USE_ETHEREUM
#error "USE_ETHEREUM is not defined"
#endif

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

void loop()
{
  if (wallet != nullptr)
  {
    // Add wallet operations here
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
  String mnemonic = inputMnemonic();
  wallet = new Wallet(mnemonic);

  String rootPrivateKey = wallet->root_private_key();
  Serial.print("Root Private Key: ");
  Serial.println(rootPrivateKey);

  String pin = inputPin();
  AES_encrypt(reinterpret_cast<const unsigned char *>(pin.c_str()),
              reinterpret_cast<const unsigned char *>(rootPrivateKey.c_str()),
              PRIVATE_KEY_SIZE, walletData.privateKey);

  walletData.initialized = true;
  EEPROM.put(0, walletData);
  EEPROM.commit();

  Serial.println("Wallet initialized:");
  Serial.print("ETH Address #0: ");
  Serial.println(Wallet::get_eth_address(wallet->derive_eth(0)));
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
      Serial.println("Wallet initialized:");
      Serial.print("ETH Address #0: ");
      Serial.println(Wallet::get_eth_address(wallet->derive_eth(0)));
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
