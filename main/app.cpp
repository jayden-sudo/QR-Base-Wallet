/**
 *
 * CONFIG_ESP_MAIN_TASK_STACK_SIZE=16384
 */

#include "app.hpp"

#define TAG "MAIN"
#define LOGI(...) ESP_LOGI(TAG, __VA_ARGS__)
#define LOGE(...) ESP_LOGE(TAG, __VA_ARGS__)
#define NVS_KEY "wallet_data_key"
#define NVS_STORAGE_NAME "storage"

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S3
// dual core: esp32, esp32-s3
#define MCU_CORE0 0
#define MCU_CORE1 1
#else
// single core: esp32-s2, esp32-c3 ...
#define MCU_CORE0 0
#define MCU_CORE1 0
#endif

// support max qr code size: 1k
#define MAX_QR_CODE_SIZE 1024
char QR_CODE_BUFFER[MAX_QR_CODE_SIZE + 1];

static Wallet *wallet = nullptr;

static bool once_after_unlock = false;
static bool on_process = false;

static bool qr_scanner_task_start = true;

static void qrScannerTask(void *parameters)
{
    bool temp_qr_scanner_task_start = false;

    camera_fb_t *fb = NULL;
    int64_t time1, time2;

    while (1)
    {
        if (temp_qr_scanner_task_start != qr_scanner_task_start)
        {
            temp_qr_scanner_task_start = qr_scanner_task_start;
            if (qr_scanner_task_start == true)
            {
                if (ESP_OK != app_camera_init())
                {
                    vTaskDelete(NULL);
                    return;
                }
            }
            else
            {
                esp_camera_deinit();
            }
        }

        if (qr_scanner_task_start == true)
        {
            fb = esp_camera_fb_get();
            if (fb == NULL)
            {
                ESP_LOGI(TAG, "camera get failed\n");
                continue;
            }
            time1 = esp_timer_get_time();
            // Decode Progress
            esp_image_scanner_t *esp_scn = esp_code_scanner_create();
            esp_code_scanner_config_t config = {ESP_CODE_SCANNER_MODE_FAST, ESP_CODE_SCANNER_IMAGE_RGB565, fb->width, fb->height};
            esp_code_scanner_set_config(esp_scn, config);
            int decoded_num = esp_code_scanner_scan_image(esp_scn, fb->buf);
            if (decoded_num)
            {
                esp_code_scanner_symbol_t result = esp_code_scanner_result(esp_scn);
                time2 = esp_timer_get_time();
                size_t data_len = strlen(result.data);
                if (data_len > MAX_QR_CODE_SIZE)
                {
                    LOGE("QR code size too large: %zu", data_len);
                    continue;
                }
                memcpy(QR_CODE_BUFFER, result.data, data_len);
                QR_CODE_BUFFER[data_len] = '\0';
                ESP_LOGI(TAG, "Decode time in %lld ms.", (time2 - time1) / 1000);
                // ESP_LOGI(TAG, "Decoded %s symbol \"%s\"\n", result.type_name, result.data);
            }
            esp_code_scanner_destroy(esp_scn);
            esp_camera_fb_return(fb);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    printf("QR-Base Wallet\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
    {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    fflush(stdout);

    xTaskCreatePinnedToCore(qrScannerTask, "qrScannerTask", 4 * 1024, NULL, 10, NULL, MCU_CORE1);

    while (1)
    {
        if (QR_CODE_BUFFER[0] != '\0')
        {
            LOGI("QR code: %s", QR_CODE_BUFFER);
            QR_CODE_BUFFER[0] = '\0';
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    WalletData walletData;

    if (!loadWalletData(walletData))
    {
        printf("Failed to load wallet data, initializing new wallet\n");
    }

    if (!walletData.initialized)
    {
        initializeWallet(walletData);
    }
    else
    {
        while (wallet == nullptr)
        {
            if (unlockWallet(walletData))
            {
                printf("Wallet unlocked successfully\n");
                break;
            }
            else
            {
                printf("Failed to unlock wallet\n");
            }
        }
    }

    printf("Setup completed\n");

    fflush(stdout);

    while (true)
    {
        loop();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

static void loop()
{
    if (wallet != nullptr)
    {
        if (once_after_unlock == false)
        {
            once_after_unlock = true;

            LOGI("ETH Address #0: ");
            LOGI("%s", Wallet::get_eth_address(wallet->derive_eth(0)).c_str());
            LOGI("ETH Address #1: ");
            LOGI("%s", Wallet::get_eth_address(wallet->derive_eth(1)).c_str());
            LOGI("ETH Address #2: ");
            LOGI("%s", Wallet::get_eth_address(wallet->derive_eth(2)).c_str());

            LOGI("Metamask Connect QR code:");
            std::string qrCode = generate_metamask_crypto_hdkey(wallet);
            printQRCode(qrCode.c_str());
            LOGI("=======================");
        }
        if (on_process == false)
        {
            on_process = true;
            {
                // std::string qrcode_input = "UR:ETH-SIGN-REQUEST/OLADTPDAGDHEWFEMLOBSMNGSFXLYASWKPDDRMEGTQDAOHDIMAOYAIOLSPKENOSLALRHKISDLAELRRPAHCTBSLFLYESMWNEVOESHLIOINKSENQZNEGMFTGYFPOSYAZMTIATIMLAROFNBYCPEOFYGOIYKTLONLAEBYCPEOFYGOIYKTLONLAEBYCPEOFYGOIYKTLONLAEBYCPEOFYGOIYKTLONLAEBYCPEOFYGOIYKTLONLAEBYCPEOFYGOIYKTLONLAERTAXAAAACYAEPKENOSAHTAADDYOEADLECSDWYKCSFNYKAEYKAEWKAEWKAOCYWLDAQZPRAMGHNEVOESHLIOINKSENQZNEGMFTGYFPOSYAZMTIATIMCNMYJTRE";
                std::string qrcode_input = "UR:ETH-SIGN-REQUEST/OLADTPDAGDBKAABWRDVOHHGEBBNEWKCAWLRPLERSKTAOHDECAOWFLSPKENOSLALRHKISDLAELPBNRODYEMDYLFGMAYMWMYIATSUTIMFHHKETHSJTWTHNCMRKWZHPTBAOEOBZLOADIAFEKSHLLEAEAELARTAXAAAACYAEPKENOSAHTAADDYOEADLECSDWYKCSFNYKAEYKAEWKAEWKAOCYWLDAQZPRAMGHNEVOESHLIOINKSENQZNEGMFTGYFPOSYAZMTIATIMFGVLDWFW";
                {
                    std::string _type;
                    std::string _payload;
                    if (decode_url(qrcode_input, &_type, &_payload) == 0)
                    {
                        if (_type == METAMASK_ETH_SIGN_REQUEST)
                        {
                            MetamaskEthSignRequest request;
                            decode_metamask_eth_sign_request(_payload, &request);

                            size_t sign_data_max_len = request.sign_data_base64url.length();
                            uint8_t *sign_data = new uint8_t[sign_data_max_len];
                            size_t sign_data_len = decode_base64url(request.sign_data_base64url, sign_data, sign_data_max_len);
                            std::string sign_data_hex = toHex(sign_data, sign_data_len);
                            LOGI("sign_data_hex:");
                            LOGI("%s", sign_data_hex.c_str());
                            LOGI("%ld", request.data_type);
                            LOGI("%lld", request.chain_id);
                            LOGI("%s", request.derivation_path.c_str());
                            LOGI("%s", request.address.c_str());
                            LOGI("Creating transaction factory...");
                            TransactionFactory *transaction = TransactionFactory::fromSerializedData(sign_data, sign_data_len);
                            do
                            {
                                if (transaction->error == 0)
                                {

                                    HDPrivateKey account = wallet->derive(request.derivation_path);
                                    std::string account_address = Wallet::get_eth_address(account);
                                    LOGI("account_address: %s", account_address.c_str());
                                    LOGI("request.address: %s", request.address.c_str());
                                    if (account_address != request.address)
                                    {
                                        LOGE("Invalid address");
                                    }
                                    else
                                    {
                                        LOGI("Signing transaction...");
                                        uint8_t signature[65];
                                        Wallet::eth_sign_serialized_data(account, sign_data, sign_data_len, signature);
                                        size_t uuid_max_len = request.uuid_base64url.length();
                                        uint8_t *uuid = new uint8_t[uuid_max_len];
                                        size_t uuid_len = decode_base64url(request.uuid_base64url, uuid, uuid_max_len);
                                        std::string qr_code = generate_metamask_eth_signature(uuid, uuid_len, signature);
                                        delete[] uuid;
                                        LOGI("Signature QR code:");
                                        // Make and print the QR Code symbol
                                        printQRCode(qr_code.c_str());
                                    }
                                }
                            } while (0);
                            delete[] sign_data;
                            delete transaction;
                        }
                        else
                        {
                            LOGE("Unsupported QR code type: ");
                            LOGI("%s", _type.c_str());
                        }
                    }
                    else
                    {

                        LOGE("Invalid QR code");
                    }
                }

                vTaskDelay(1000 * 60 / portTICK_PERIOD_MS);
            }
            on_process = false;
        }
    }
}

static std::unique_ptr<nvs::NVSHandle> initNVSHandle()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(NVS_STORAGE_NAME, NVS_READWRITE, &err);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return nullptr;
    }
    return handle;
}

bool loadWalletData(WalletData &walletData)
{
    auto handle = initNVSHandle();
    if (!handle)
        return false;

    char _data[sizeof(WalletData)];
    esp_err_t err = handle->get_blob(NVS_KEY, _data, sizeof(WalletData));
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            printf("The value is not initialized yet!\n");
        }
        else
        {
            printf("Error (%s) reading wallet data!\n", esp_err_to_name(err));
        }
        return false;
    }
    memcpy(&walletData, _data, sizeof(WalletData));
    return true;
}
bool storeWalletData(WalletData &walletData)
{
    auto handle = initNVSHandle();
    if (!handle)
        return false;

    char _data[sizeof(WalletData)];
    memcpy(_data, &walletData, sizeof(WalletData));
    esp_err_t err = handle->set_blob(NVS_KEY, _data, sizeof(WalletData));
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            printf("The value is not initialized yet!\n");
        }
        else
        {
            printf("Error (%s) reading wallet data!\n", esp_err_to_name(err));
        }
        return false;
    }
    err = handle->commit();
    if (err != ESP_OK)
    {
        printf("Error (%s) storing wallet data!\n", esp_err_to_name(err));
        return false;
    }
    return true;
}

static std::string inputMnemonic()
{
    std::string mnemonic = "until exhaust file evidence reopen mad stumble beach acquire judge fuel raccoon cram arrange sugar swim cluster exile picture curtain velvet choice surge aware";
    LOGI("mnemonic: %s", mnemonic.c_str());
    return mnemonic;
}
static std::string inputPin()
{
    std::string pin = "123456";
    LOGI("pin: %s", pin.c_str());
    return pin;
}
static void initializeWallet(WalletData &walletData)
{
    if (wallet != nullptr)
    {
        // can't initialize wallet twice
        return;
    }
    std::string mnemonic = inputMnemonic();
    Wallet *_wallet = new Wallet(mnemonic);
    std::string rootPrivateKey = _wallet->root_private_key();
    std::string pin = inputPin();
    aes_encrypt(reinterpret_cast<const unsigned char *>(pin.c_str()),
                reinterpret_cast<const unsigned char *>(rootPrivateKey.c_str()),
                PRIVATE_KEY_SIZE, walletData.privateKey);
    if (!storeWalletData(walletData))
    {
        printf("Failed to store wallet data, restarting...\n");
        fflush(stdout);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
        return;
    }
    wallet = _wallet;
    walletData.initialized = true;
    printf("Wallet initialized\n");
}
static bool unlockWallet(WalletData &walletData)
{
    while (true)
    {
        std::string pin = inputPin();
        char rootPrivateKey[PRIVATE_KEY_SIZE] = {0};
        aes_decrypt(reinterpret_cast<const unsigned char *>(pin.c_str()),
                    walletData.privateKey, PRIVATE_KEY_SIZE,
                    reinterpret_cast<unsigned char *>(rootPrivateKey));

        std::string rootPrivateKeyStr = std::string(rootPrivateKey);
        if (rootPrivateKeyStr.starts_with("xprv"))
        {
            if (walletData.incorrectPinCount > 0)
            {
                walletData.incorrectPinCount = 0;
                if (!storeWalletData(walletData))
                {
                    printf("Failed to store wallet data, restarting...\n");
                    fflush(stdout);
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    esp_restart();
                    return false;
                }
            }
            wallet = new Wallet(rootPrivateKey);
            printf("Wallet unlocked\n");
            return true;
        }

        walletData.incorrectPinCount++;
        if (walletData.incorrectPinCount >= MAX_INCORRECT_PIN_ATTEMPTS)
        {
            printf("Incorrect PIN count >= 3, erasing wallet data.\n");
            eraseWalletData(walletData);
            return false;
        }

        printf("Incorrect PIN, try again.\n");
        if (!storeWalletData(walletData))
        {
            printf("Failed to store wallet data, restarting...\n");
            fflush(stdout);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart();
            return false;
        }
    }
}
static void eraseWalletData(WalletData &walletData)
{
    walletData.initialized = false;
    walletData.incorrectPinCount = 0;
    memset(walletData.privateKey, 0, sizeof(walletData.privateKey));
    if (!storeWalletData(walletData))
    {
        printf("Failed to store wallet data, restarting...\n");
        fflush(stdout);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
        return;
    }
}
static uint8_t QRCODE_BUFFER[qrcodegen_BUFFER_LEN_MAX];
static uint8_t QRCODE_TEMP_BUFFER[qrcodegen_BUFFER_LEN_MAX];
static void printQRCode(const char *qrcode_string)
{
    enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW;
    bool ok = qrcodegen_encodeText(qrcode_string, QRCODE_TEMP_BUFFER, QRCODE_BUFFER, errCorLvl, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
    if (ok)
    {
        int size = qrcodegen_getSize(QRCODE_BUFFER);
        int border = 4;
        for (int y = -border; y < size + border; y++)
        {
            for (int x = -border; x < size + border; x++)
            {
                printf(qrcodegen_getModule(QRCODE_BUFFER, x, y) ? "██" : "  ");
            }
            printf("\n");
        }
        printf("\n");
    }
    else
    {
        LOGE("Failed to print QR code");
    }
}