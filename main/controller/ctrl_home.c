/*********************
 *      INCLUDES
 *********************/
#include "controller/ctrl_home.h"
#include <esp_system.h>
#include <esp_log.h>
#include <esp_random.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "kv_fs.h"
#include <string.h>
#include "ui/ui_home.h"
#include "qrcode_protocol.h"
#include "wallet.h"
#include "crc32.h"
#include "app_peripherals.h"
#include "sha256_str.h"
#include "qrcode_protocol.h"
#include "esp_code_scanner.h"
#include "controller/ctrl_sign.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static const char *TAG = "ctrl_home";
static Wallet *wallet = NULL;
static ctrl_home_network_data *network_data = NULL;
static int *flag = NULL;
static char *privateKeyStr = NULL;
static char *pinStr = NULL;
static bool scan_task_running = 0;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void qrScannerTask(void *parameters);

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void ctrl_home_init(char *_privateKeyStr, char *_pinStr, int *_flag);
void ctrl_home_free(void);
bool ctrl_home_verify_pin(char *pinStr);

/* wallet page */
ctrl_home_network_data *ctrl_home_list_networks(void);
char *ctrl_home_get_connect_qrcode(ctrl_home_network_data *network, ctrl_home_connect_qr_type qr_type);

/* scanner page */
void ctrl_home_scan_qr_start(lv_obj_t *canvas);
void ctrl_home_scan_qr_stop(void);

/* settings page */
bool ctrl_home_lock(void);
bool ctrl_home_erase_wallet(void);
int ctrl_home_pin_max_attempts_get(void);
bool ctrl_home_pin_max_attempts_set(int max_attempts);

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void qrScannerTask(void *parameters)
{
    lv_obj_t *image = (lv_obj_t *)parameters;
    if (ESP_OK != app_camera_init())
    {
        return;
    }

    lv_img_dsc_t img_buffer = {
        .header.w = 0,
        .header.h = 0,
        .data_size = 0,
        .header.cf = LV_COLOR_FORMAT_RGB565,
        .data = NULL,
    };
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb == NULL)
    {
        ESP_LOGE(TAG, "camera get failed");
        vTaskDelete(NULL);
        return;
    }
    if (fb->width != 240 || fb->height != 240)
    {
        ESP_LOGE(TAG, "camera not in 240x240");
        vTaskDelete(NULL);
        return;
    }

    qrcode_protocol_bc_ur_data_t *qrcode_protocol_bc_ur_data = (qrcode_protocol_bc_ur_data_t *)malloc(sizeof(qrcode_protocol_bc_ur_data_t));
    qrcode_protocol_bc_ur_init(qrcode_protocol_bc_ur_data);

    img_buffer.header.w = fb->width;
    img_buffer.header.h = fb->height;
    img_buffer.data_size = fb->len;
    esp_code_scanner_config_t config = {ESP_CODE_SCANNER_MODE_FAST, ESP_CODE_SCANNER_IMAGE_RGB565, fb->width, fb->height};
    esp_camera_fb_return(fb);

    bool scan_success = false;

    while (scan_task_running && !scan_success)
    {
        fb = esp_camera_fb_get();
        if (fb == NULL)
        {
            ESP_LOGE(TAG, "camera get failed");
            continue;
        }
        img_buffer.data = fb->buf;
        if (lvgl_port_lock(0))
        {
            lv_img_set_src(image, &img_buffer);
            lvgl_port_unlock();
        }
        {
            // Decode Progress
            esp_image_scanner_t *esp_scn = esp_code_scanner_create();
            esp_code_scanner_set_config(esp_scn, config);
            int decoded_num = esp_code_scanner_scan_image(esp_scn, fb->buf);
            if (decoded_num)
            {
                esp_code_scanner_symbol_t result = esp_code_scanner_result(esp_scn);
                if (result.data != NULL && strlen(result.data) > 0)
                {
                    // Decode UR
                    qrcode_protocol_bc_ur_receive(qrcode_protocol_bc_ur_data, result.data);
                    if (qrcode_protocol_bc_ur_is_success(qrcode_protocol_bc_ur_data))
                    {
                        // ESP_LOGI(TAG, "scan success");
                        scan_success = true;
                        ui_home_stop_qr_scan();
                        ctrl_sign_init(wallet, qrcode_protocol_bc_ur_data);
                    }
                }
            }
            /* esp_code_scanner_symbol_t unavailable after esp_code_scanner_destroy */
            esp_code_scanner_destroy(esp_scn);
        }
        esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (!scan_success)
    {
        qrcode_protocol_bc_ur_free(qrcode_protocol_bc_ur_data);
        free(qrcode_protocol_bc_ur_data);
        qrcode_protocol_bc_ur_data = NULL;
    } // if scan success, ctrl_sign_init will free qrcode_protocol_bc_ur_data

    if (lvgl_port_lock(0))
    {
        lv_img_set_src(image, NULL);
        lvgl_port_unlock();
    }
    esp_camera_deinit();
    vTaskDelete(NULL);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void ctrl_home_init(char *_privateKeyStr, char *_pinStr, int *_flag)
{
    privateKeyStr = _privateKeyStr;
    pinStr = _pinStr;
    flag = _flag;

    wallet = wallet_init_from_xprv(privateKeyStr);

    ui_home_create();
}
void ctrl_home_free(void)
{
    if (wallet != NULL)
    {
        wallet_free(wallet);
        wallet = NULL;
    }
    if (network_data != NULL)
    {
        ctrl_home_network_data *network_current = network_data;
        ctrl_home_network_data *network_next;
        while (network_current != NULL)
        {
            network_next = network_current->next;
            // free current
            wallet_free(network_current->wallet_current);
            // free compatible_wallet_group
            {
                ctrl_home_compatible_wallet_group *compatible_wallet_group = network_current->compatible_wallet_group;
                // free wallet_info_3rd
                {
                    ctrl_home_3rd_wallet_info *wallet_info_3rd_current = compatible_wallet_group->wallet_info_3rd;
                    ctrl_home_3rd_wallet_info *wallet_info_3rd_next;
                    while (wallet_info_3rd_current != NULL)
                    {
                        wallet_info_3rd_next = wallet_info_3rd_current->next;
                        free(wallet_info_3rd_current);
                        wallet_info_3rd_current = wallet_info_3rd_next;
                    }
                }
                free(compatible_wallet_group);
            }
        }
        free(network_current);
        network_current = network_next;

        network_data = NULL;
    }
}

bool ctrl_home_verify_pin(char *pinStr)
{
    return strcmp(pinStr, pinStr) == 0;
}

/* wallet page */
ctrl_home_network_data *ctrl_home_list_networks(void)
{
    if (network_data == NULL)
    {
        ctrl_home_network_data *network_data_temp = NULL;

        {
            network_data_temp = (ctrl_home_network_data *)malloc(sizeof(ctrl_home_network_data));
            memset(network_data_temp, 0, sizeof(ctrl_home_network_data));
            {
                network_data_temp->type = CTRL_HOME_NETWORK_TYPE_ETH;
                network_data_temp->icon = NULL;
                strcpy(network_data_temp->name, "Ethereum");
                Wallet *_wallet = wallet_derive_eth(wallet, 0);
                char walletAddress[43];
                wallet_get_eth_address(_wallet, walletAddress);
                strcpy(network_data_temp->address, walletAddress);
                network_data_temp->wallet_main = wallet;
                network_data_temp->wallet_current = _wallet;
                ctrl_home_compatible_wallet_group *compatible_wallet_group = (ctrl_home_compatible_wallet_group *)malloc(sizeof(ctrl_home_compatible_wallet_group));
                {
                    memset(compatible_wallet_group, 0, sizeof(ctrl_home_compatible_wallet_group));
                    compatible_wallet_group->qr_type = CTRL_HOME_CONNECT_QR_TYPE_METAMASK;
                    ctrl_home_3rd_wallet_info *wallet_info_3rd = (ctrl_home_3rd_wallet_info *)malloc(sizeof(ctrl_home_3rd_wallet_info));
                    {
                        memset(wallet_info_3rd, 0, sizeof(ctrl_home_3rd_wallet_info));
                        strcpy(wallet_info_3rd->name, "MetaMask");
                        wallet_info_3rd->icon = NULL;
                        wallet_info_3rd->next = NULL;
                    }
                    compatible_wallet_group->wallet_info_3rd = wallet_info_3rd;
                    compatible_wallet_group->next = NULL;
                }
                network_data_temp->compatible_wallet_group = compatible_wallet_group;
                network_data_temp->next = NULL;
            }
        }

        ctrl_home_network_data *network_data_ethereum = network_data_temp;

        {
            network_data_temp = (ctrl_home_network_data *)malloc(sizeof(ctrl_home_network_data));
            memset(network_data_temp, 0, sizeof(ctrl_home_network_data));
            {
                network_data_temp->type = CTRL_HOME_NETWORK_TYPE_ETH;
                network_data_temp->icon = NULL;
                strcpy(network_data_temp->name, "Bitcoin segwit");
                Wallet *_wallet = wallet_derive_btc(wallet, 0);
                char walletAddress[43];
                wallet_get_btc_address_segwit(_wallet, walletAddress);
                strcpy(network_data_temp->address, walletAddress);
                network_data_temp->wallet_main = wallet;
                network_data_temp->wallet_current = _wallet;
                network_data_temp->compatible_wallet_group = NULL;
                network_data_temp->next = NULL;
            }
        }

        ctrl_home_network_data *network_data_bitcoin_segwit = network_data_temp;

        network_data_ethereum->next = network_data_bitcoin_segwit;

        network_data = network_data_ethereum;
    }
    return network_data;
}
char *ctrl_home_get_connect_qrcode(ctrl_home_network_data *network, ctrl_home_connect_qr_type qr_type)
{
    char *hdkey = NULL;
    generate_metamask_crypto_hdkey(network->wallet_main, &hdkey);
    return hdkey;
}

/* scanner page */
void ctrl_home_scan_qr_start(lv_obj_t *image)
{
    scan_task_running = true;
    xTaskCreatePinnedToCore(qrScannerTask, "qrScannerTask", 4 * 1024, image, 10, NULL, MCU_CORE1);
}
void ctrl_home_scan_qr_stop(void)
{
    scan_task_running = false;
}

/* settings page */
bool ctrl_home_lock(void)
{
    return false;
}
bool ctrl_home_erase_wallet(void)
{
    return false;
}
int ctrl_home_pin_max_attempts_get(void)
{
    return 0;
}
bool ctrl_home_pin_max_attempts_set(int max_attempts)
{
    return false;
}