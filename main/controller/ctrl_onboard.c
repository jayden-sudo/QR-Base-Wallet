/*********************
 *      INCLUDES
 *********************/
#include "controller/ctrl_onboard.h"
#include <esp_system.h>
#include <esp_log.h>
#include <esp_random.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "kv_fs.h"
#include <string.h>
#include "ui/ui_loading.h"
#include "ui/ui_phrase_input_page.h"
#include "ui/ui_pin_input_page.h"
#include "qrcode_protocol.h"
#include "wallet.h"
#include "crc32.h"
#include "app_peripherals.h"
#include "sha256_str.h"
#include "aes_str.h"

/*********************
 *      DEFINES
 *********************/
#define PRIVATE_KEY_SIZE AES_BLOCK_SIZE * 8
#define DELAY_MS_MAX 1000
#define DELAY_MS_MIN 5
#define DEFAULT_INCORRECT_PIN_COUNT 5
#define VERSION 1

/**********************
 *      TYPEDEFS
 **********************/
typedef enum
{
    STEP_INIT_SET_PHRASE = 0,
    STEP_INIT_SET_PIN,
    STEP_VERIFY_PIN,
} UI_STEP;

typedef struct __attribute__((aligned(4)))
{
    int version;
    bool initialized;
    uint8_t incorrectPinCount;
    uint8_t incorrectPinCountMax;
    uint8_t pinPadding[32];
    char privateKey[PRIVATE_KEY_SIZE];
    uint32_t checksum;
} WALLET_DATA;

/**********************
 *      MACROS
 **********************/
#define SEMAPHORE_GIVE(___rootPrivateKeyStr, ___pinStr)        \
    do                                                         \
    {                                                          \
        *_privateKeyStr = malloc(PRIVATE_KEY_SIZE + 1);        \
        *_pinStr = malloc(10);                                 \
        strcpy(*_privateKeyStr, (char *)___rootPrivateKeyStr); \
        strcpy(*_pinStr, (char *)___pinStr);                   \
    } while (0);

/**********************
 *  STATIC VARIABLES
 **********************/
static const char *TAG = "ctrl_onboard";
// static SemaphoreHandle_t *xSemaphorePtr = NULL;
static char **_privateKeyStr = NULL;
static char **_pinStr = NULL;

static UI_STEP current_step;
static char *phrase_cache = NULL;
static char *pin_cache = NULL;
static WALLET_DATA *walletData_cache = NULL;
static char temp[50];
// static QueueHandle_t result_queue;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void pin_avoid_rainbow_table(const char *pinStr, const unsigned char padding[32], unsigned char key[32]);
static size_t wallet_data_to_bin(WALLET_DATA *walletData, char **hex);
static void wallet_data_from_bin(WALLET_DATA *walletData, const char *hex, size_t len);
static void save_wallet_data(WALLET_DATA *walletData);
static void ui_event_handler(lv_event_t *e);
static char *verify_pin(char *pin);
static uint32_t checksum(WALLET_DATA *walletData);
static void task_init(void *parameters);
static void panic_with_message(const char *message);
static void panic_event_cb(lv_event_t *e);
static void show_pin_verify_page(WALLET_DATA *walletData);

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void ctrl_onboard(char **privateKeyStr, char **pinStr);

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void pin_avoid_rainbow_table(const char *pinStr, const unsigned char padding[32], unsigned char key[32])
{
    size_t len = sizeof(char) * (strlen(pinStr) + 32 + 1);
    char *str = malloc(len);
    if (str == NULL)
    {
        ESP_LOGE(TAG, "malloc failed");
        abort();
    }
    strcpy(str, pinStr);
    strncpy((char *)str + strlen(pinStr), (char *)padding, 32);
    str[len - 1] = '\0';
    sha256_str(str, key);
    free(str);
    str = NULL;
}
static size_t wallet_data_to_bin(WALLET_DATA *walletData, char **hex)
{
    size_t size = sizeof(WALLET_DATA);
    *hex = (char *)malloc(size);
    if (*hex == NULL)
    {
        ESP_LOGE(TAG, "malloc failed");
        abort();
    }
    memcpy(*hex, walletData, size);
    return size;
}
static void wallet_data_from_bin(WALLET_DATA *walletData, const char *hex, size_t len)
{
    size_t size = sizeof(WALLET_DATA);
    if (len < size)
    {
        ESP_LOGE(TAG, "len < sizeof(WALLET_DATA)");
        memset(walletData, 0, size);
    }
    else
    {
        memcpy(walletData, hex, size);
    }
}
static bool load_wallet_data(WALLET_DATA *walletData)
{
    char *hex = NULL;
    size_t len = 0;
    int ret = kv_load(KV_FS_KEY_WALLET, &hex, &len);
    if (ret != ESP_OK)
    {
        panic_with_message("kv_load failed");
    }

    wallet_data_from_bin(walletData, hex, len);
    if (hex != NULL)
    {
        free(hex);
        hex = NULL;
    }
    if (walletData->initialized == true)
    {
        // checksum
        int checksum_value = checksum(walletData);
        if (walletData->checksum != checksum_value)
        {
            kv_delete(KV_FS_KEY_WALLET);
            panic_with_message("Checksum error, reset wallet...");
        }
        else
        {
            return true;
        }
    }
    return false;
}
static void save_wallet_data(WALLET_DATA *walletData)
{
    if (walletData->initialized == false)
    {
        panic_with_message("walletData is not initialized");
        return;
    }
    walletData->checksum = checksum(walletData);
    char *walletDataStr = NULL;
    size_t len = wallet_data_to_bin(walletData, &walletDataStr);
    if (kv_save(KV_FS_KEY_WALLET, walletDataStr, len) != ESP_OK)
    {
        panic_with_message("kv_save failed");
        return;
    }
    free(walletDataStr);
    walletDataStr = NULL;
}
static void task_init(void *parameters)
{
    lvgl_port_lock(0);
    ui_loading_show();
    lvgl_port_unlock();

    // int result = 0;

    {
        Wallet wallet = wallet_init_from_mnemonic(phrase_cache);
        char *root_private_key = wallet_root_private_key(wallet);
        WALLET_DATA *walletData = malloc(sizeof(WALLET_DATA));
        memset(walletData, 0, sizeof(WALLET_DATA));
        esp_fill_random(walletData->pinPadding, 32);
        unsigned char *key = malloc(32);
        pin_avoid_rainbow_table(pin_cache, walletData->pinPadding, key);
        aes_encrypt(key,
                    (unsigned char *)root_private_key,
                    PRIVATE_KEY_SIZE, (unsigned char *)(walletData->privateKey));
        // test decrypt
        {
            unsigned char rootPrivateKey[PRIVATE_KEY_SIZE + 1];
            aes_decrypt(key,
                        (unsigned char *)(walletData->privateKey), PRIVATE_KEY_SIZE,
                        rootPrivateKey);
            if (strcmp(root_private_key, (char *)rootPrivateKey) != 0)
            {
                panic_with_message("decrypt failed");
            }
            if (strncmp((char *)rootPrivateKey, "xprv", 4) != 0)
            {
                panic_with_message("decrypt failed");
            }
        }
        free(key);
        key = NULL;

        walletData->initialized = true;
        walletData->version = VERSION;
        walletData->incorrectPinCount = 0;
        walletData->incorrectPinCountMax = DEFAULT_INCORRECT_PIN_COUNT;
        save_wallet_data(walletData);
        // read from storage test
        WALLET_DATA *walletData_read = malloc(sizeof(WALLET_DATA));
        if (load_wallet_data(walletData_read) == false)
        {
            panic_with_message("load_wallet_data failed");
            return;
        }
        if (walletData_read->initialized == false)
        {
            panic_with_message("walletData_read is not initialized");
            return;
        }
        if (strncmp(walletData_read->privateKey, walletData->privateKey, PRIVATE_KEY_SIZE) != 0)
        {
            panic_with_message("walletData_read privateKey not match");
            return;
        }
        free(walletData);
        walletData = NULL;
        free(walletData_read);
        walletData_read = NULL;
        wallet_free(wallet);
        wallet = NULL;
        SEMAPHORE_GIVE(root_private_key, pin_cache);
        free(root_private_key);
        root_private_key = NULL;
    }

    lvgl_port_lock(0);
    ui_loading_hide();
    lvgl_port_unlock();

    free(phrase_cache);
    phrase_cache = NULL;
    free(pin_cache);
    pin_cache = NULL;

    // xQueueSend(result_queue, &result, portMAX_DELAY);
    vTaskDelete(NULL);
}
static void ui_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == UI_EVENT_PHRASE_CONFIRM)
    {
        char *phrase = lv_event_get_param(e);
        if (current_step == STEP_INIT_SET_PHRASE)
        {
            // first time setup
            phrase_cache = phrase;
            // enter pin
            current_step = STEP_INIT_SET_PIN;
            ui_create_pin_input_page_set(lv_scr_act(), false);
        }
    }
    else if (code == UI_EVENT_PHRASE_CANCEL)
    {
        printf("event: UI_EVENT_PHRASE_CANCEL\n");
        // create_main_ui();
    }
    else if (code == UI_EVENT_PIN_CONFIRM)
    {
        char *pin = lv_event_get_param(e);
        if (current_step == STEP_INIT_SET_PIN)
        {
            pin_cache = pin;
            // result_queue = xQueueCreate(1, sizeof(int));
            // if (result_queue == NULL)
            // {
            //     ESP_LOGE(TAG, "xQueueCreate failed");
            //     abort();
            // }
            xTaskCreatePinnedToCore(task_init, "task_init", 4 * 1024, NULL, 10, NULL, MCU_CORE1);

            // int result;
            // if (xQueueReceive(result_queue, &result, portMAX_DELAY))
            // {
            //     ESP_LOGI(TAG, "Task completed with result: %d\n", result);
            // }
            // else
            // {
            //     ESP_LOGE(TAG, "xQueueReceive failed");
            //     abort();
            // }

            // vQueueDelete(result_queue);
            // result_queue = NULL;

            // free(phrase_cache);
            // phrase_cache = NULL;
            // free(pin_cache);
            // pin_cache = NULL;
        }
        else if (STEP_VERIFY_PIN == current_step)
        {
            // skip
        }
    }
    else if (code == UI_EVENT_PIN_CANCEL)
    {
        printf("event: %d\n", lv_event_get_code(e));
        // create_main_ui();
    }
}
static uint32_t checksum(WALLET_DATA *walletData)
{
    // copy walletData to m,except `uint32_t checksum`
    size_t size = sizeof(WALLET_DATA) - sizeof(uint32_t);
    unsigned char *m = (unsigned char *)malloc(size);
    memcpy(m, walletData, size);
    uint32_t c = crc32(0, m, size);
    free(m);
    return c;
}
static char *verify_pin(char *pin)
{
    // return NULL if pin is correct

    if (STEP_VERIFY_PIN == current_step)
    {
        if (walletData_cache == NULL)
        {
            panic_with_message("walletData_cache is NULL");
            return NULL;
        }
        if (walletData_cache->initialized == false)
        {
            panic_with_message("walletData_cache is not initialized");
            return NULL;
        }
        if (walletData_cache->version != VERSION)
        {
            // TODO: version merge
            panic_with_message("version not match");
            return NULL;
        }
        // check pin
        unsigned char *key = malloc(32);
        pin_avoid_rainbow_table(pin, walletData_cache->pinPadding, key);
        unsigned char rootPrivateKey[PRIVATE_KEY_SIZE + 1];
        aes_decrypt(key,
                    (unsigned char *)(walletData_cache->privateKey), PRIVATE_KEY_SIZE,
                    rootPrivateKey);
        rootPrivateKey[PRIVATE_KEY_SIZE] = '\0';
        free(key);
        key = NULL;
        if (strncmp((char *)rootPrivateKey, "xprv", 4) != 0)
        {
            walletData_cache->incorrectPinCount++;
            if (walletData_cache->incorrectPinCount >= walletData_cache->incorrectPinCountMax)
            {
                kv_delete(KV_FS_KEY_WALLET);
                panic_with_message("Too many incorrect passcode attempts. Device reset.");
                return NULL;
            }
            // save to storage
            save_wallet_data(walletData_cache);

            sprintf(temp, "Passcode error %d/%d", (walletData_cache->incorrectPinCount) + 1, walletData_cache->incorrectPinCountMax);
            return temp;
        }
        else
        {
            if (walletData_cache->incorrectPinCount > 0)
            {
                walletData_cache->incorrectPinCount = 0;
                save_wallet_data(walletData_cache);
            }
            SEMAPHORE_GIVE(rootPrivateKey, pin);
            /* verify pin success */
            return NULL;
        }
    }
    else
    {
        ESP_LOGE(TAG, "ERROR step: %d", current_step);
        return "Unknown error";
    }
}
static void panic_event_cb(lv_event_t *e)
{
    esp_restart();
}
static void panic_with_message(const char *message)
{
    ESP_LOGE(TAG, "%s", message);
    lv_obj_t *mbox1 = lv_msgbox_create(NULL);
    lv_obj_set_size(mbox1, lv_obj_get_content_width(lv_scr_act()), LV_SIZE_CONTENT);
    lv_msgbox_add_title(mbox1, "Panic - can't recover");
    lv_msgbox_add_text(mbox1, message);

    lv_obj_t *btn;
    btn = lv_msgbox_add_footer_button(mbox1, "Reboot!");
    lv_obj_add_event_cb(btn, panic_event_cb, LV_EVENT_CLICKED, NULL);
}
static void show_pin_verify_page(WALLET_DATA *walletData)
{
    if (walletData->incorrectPinCount > 0)
    {
        sprintf(temp, "Passcode error %d/%d", (walletData->incorrectPinCount) + 1, walletData->incorrectPinCountMax);
        ui_create_pin_input_page_verify(lv_scr_act(), false, temp, verify_pin);
    }
    else
    {
        ui_create_pin_input_page_verify(lv_scr_act(), false, NULL, verify_pin);
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void ctrl_onboard(char **privateKeyStr, char **pinStr)
{
    _privateKeyStr = privateKeyStr;
    _pinStr = pinStr;
    /* Task lock */
    /* show loading */
    ui_loading_show();
    current_step = STEP_INIT_SET_PHRASE;
    walletData_cache = malloc(sizeof(WALLET_DATA));
    memset(walletData_cache, 0, sizeof(WALLET_DATA));
    lv_obj_t *screen = lv_scr_act();
    ui_init_events();
    lv_obj_add_event_cb(screen, ui_event_handler, UI_EVENT_PHRASE_CONFIRM, NULL);
    lv_obj_add_event_cb(screen, ui_event_handler, UI_EVENT_PHRASE_CANCEL, NULL);
    lv_obj_add_event_cb(screen, ui_event_handler, UI_EVENT_PIN_CONFIRM, NULL);
    lv_obj_add_event_cb(screen, ui_event_handler, UI_EVENT_PIN_CANCEL, NULL);

    /* check if wallet is initialized */
    bool initialized = load_wallet_data(walletData_cache);

    /* hide loading */
    ui_loading_hide();

    if (!initialized)
    {
        current_step = STEP_INIT_SET_PHRASE;
        free(walletData_cache);
        walletData_cache = NULL;
        /* show phrase input page */
        ui_create_phrase_input_page(screen, false);
    }
    else
    {
        current_step = STEP_VERIFY_PIN;
        /* show pin verify page */
        show_pin_verify_page(walletData_cache);
    }
}