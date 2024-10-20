#ifndef CTRL_HOME_H
#define CTRL_HOME_H

/*********************
 *      INCLUDES
 *********************/
#include "esp_lvgl_port.h"
#include "wallet.h"
#include "esp_camera.h"

/**********************
 *      TYPEDEFS
 **********************/
typedef enum
{
    CTRL_HOME_NETWORK_TYPE_ETH = 0,
    CTRL_HOME_NETWORK_TYPE_BTC_SEGWIT = 1,
} ctrl_home_network_type;

typedef enum
{
    CTRL_HOME_CONNECT_QR_TYPE_METAMASK = 0,
    CTRL_HOME_CONNECT_QR_TYPE_TODO = 1,
} ctrl_home_connect_qr_type;

typedef struct __attribute__((aligned(4))) _ctrl_home_3rd_wallet_info
{
    /* 3rd wallet icon */
    lv_image_dsc_t *icon;
    /* 3rd wallet name */
    char name[32];
    /* next */
    struct _ctrl_home_3rd_wallet_info *next;
} ctrl_home_3rd_wallet_info;

typedef struct __attribute__((aligned(4))) _ctrl_home_compatible_wallet_group
{
    /* qr type */
    ctrl_home_connect_qr_type qr_type;
    /* 3rd wallet list */
    ctrl_home_3rd_wallet_info *wallet_info_3rd;
    /* next group */
    struct _ctrl_home_compatible_wallet_group *next;
} ctrl_home_compatible_wallet_group;

typedef struct __attribute__((aligned(4))) _ctrl_home_network_data
{
    /* network type */
    ctrl_home_network_type type;
    /* network icon */
    lv_image_dsc_t *icon;
    /* network name */
    char name[32];
    /* wallet address */
    char address[50];
    /* main wallet */
    Wallet *wallet_main;
    /* current wallet */
    Wallet *wallet_current;
    /* compatible wallet group */
    ctrl_home_compatible_wallet_group *compatible_wallet_group;
    /* next network */
    struct _ctrl_home_network_data *next;
} ctrl_home_network_data;

#ifdef __cplusplus
extern "C"
{
#endif

    /**********************
     * GLOBAL PROTOTYPES
     **********************/
    void ctrl_home_init(char *privateKeyStr, char *pinStr, int *flag);
    void ctrl_home_free(void);
    bool ctrl_home_verify_pin(char *pinStr);

    /* wallet page */
    ctrl_home_network_data *ctrl_home_list_networks(void);
    char *ctrl_home_get_connect_qrcode(ctrl_home_network_data *network, ctrl_home_connect_qr_type qr_type);

    /* scanner page */
    void ctrl_home_scan_qr_start(lv_obj_t *image);
    void ctrl_home_scan_qr_stop(void);

    /* settings page */
    bool ctrl_home_lock(void);
    bool ctrl_home_erase_wallet(void);
    int ctrl_home_pin_max_attempts_get(void);
    bool ctrl_home_pin_max_attempts_set(int max_attempts);

#ifdef __cplusplus
}
#endif

#endif /* CTRL_HOME_H */