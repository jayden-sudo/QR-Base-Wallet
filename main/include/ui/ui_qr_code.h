#ifndef UI_QR_CODE_H
#define UI_QR_CODE_H

/*********************
 *      INCLUDES
 *********************/
#include "esp_lvgl_port.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**********************
     * GLOBAL PROTOTYPES
     **********************/
    int ui_qr_code_show(lv_obj_t *canvas, int max_width, const char *qr_code_str, bool sync);

#ifdef __cplusplus
    extern "C"
}
#endif

#endif /* UI_QR_CODE_H */
