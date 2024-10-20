#ifndef CTRL_SIGN_H
#define CTRL_SIGN_H

/*********************
 *      INCLUDES
 *********************/
#include "esp_lvgl_port.h"
#include "qrcode_protocol.h"
#include "wallet.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**********************
     * GLOBAL PROTOTYPES
     **********************/
    void ctrl_sign_init(Wallet *wallet, qrcode_protocol_bc_ur_data_t *qrcode_protocol_bc_ur_data);
    void ctrl_sign_free(void);
    char *ctrl_sign_get_signature(void);

#ifdef __cplusplus
}
#endif

#endif /* CTRL_SIGN_H */