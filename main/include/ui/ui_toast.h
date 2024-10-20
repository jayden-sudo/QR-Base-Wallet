#ifndef UI_TOAST_H
#define UI_TOAST_H

/*********************
 *      INCLUDES
 *********************/
#include "esp_lvgl_port.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**********************
     * GLOBAL PROTOTYPES
     **********************/
    void ui_toast_show(const char *text, int duration);

#ifdef __cplusplus
    extern "C"
}
#endif

#endif /* UI_TOAST_H */