#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_timer.h"
#include "linenoise/linenoise.h"
#include <utility/trezor/bip39.h>
#include "crc32.h"
#include "aes.h"
#include "wallet.hpp"
#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_handle.hpp"
#include <iostream>
#include "qrcode_protocol.hpp"
#include "base64url.hpp"
#include "qrcodegen.h"
#include "cbor.h"
#include "app_peripherals.h"
#include "esp_code_scanner.h"

#define MAX_INCORRECT_PIN_ATTEMPTS 3
#define MIN_PIN_LENGTH 3
#define MAX_PIN_LENGTH 10
#define PRIVATE_KEY_SIZE AES_BLOCK_SIZE * 8

extern "C"
{
    struct WalletData
    {
        uint32_t checksum = 0;
        bool initialized = false;
        uint8_t incorrectPinCount = 0;
        unsigned char privateKey[PRIVATE_KEY_SIZE] = {0};
    };

    void app_main(void);
}

static std::string inputMnemonic();
static std::string inputPin();

static void loop();
static bool loadWalletData(WalletData &walletData);
static bool storeWalletData(WalletData &walletData);

static void initializeWallet(WalletData &walletData);
static bool unlockWallet(WalletData &walletData);
static void eraseWalletData(WalletData &walletData);

static void qrScannerTask(void *parameters);

static void printQRCode(const char *qrcode);

static std::unique_ptr<ur::UR> startURScanner();