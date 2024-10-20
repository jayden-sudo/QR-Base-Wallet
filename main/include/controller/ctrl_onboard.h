#ifndef CTRL_ONBOARD_H
#define CTRL_ONBOARD_H

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
    void ctrl_onboard(char **privateKeyStr, char **pinStr);

#ifdef __cplusplus
}
#endif

#endif /* CTRL_ONBOARD_H */