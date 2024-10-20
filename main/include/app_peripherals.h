#ifndef APP_PERIPHERALS_H
#define APP_PERIPHERALS_H

/*********************
 *      INCLUDES
 *********************/
#include "esp_err.h"

/*********************
 *      DEFINES
 *********************/
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S3
// dual core: esp32, esp32-s3
#define MCU_CORE0 0
#define MCU_CORE1 1
#else
// single core: esp32-s2, esp32-c3 ...
#define MCU_CORE0 0
#define MCU_CORE1 0
#endif

// // set button pins here
// #define BUTTON_NUM 4
// #define BUTTON_DEBOUNCE_TIME 50
// #define GPIO_BUTTON_CANCEL GPIO_NUM_4
// #define GPIO_BUTTON_UP GPIO_NUM_5
// #define GPIO_BUTTON_DOWN GPIO_NUM_6
// #define GPIO_BUTTON_CONFIRM GPIO_NUM_7
// const int BUTTON_GPIOS[BUTTON_NUM] = {GPIO_BUTTON_CANCEL, GPIO_BUTTON_UP, GPIO_BUTTON_DOWN, GPIO_BUTTON_CONFIRM};

#ifdef __cplusplus
extern "C"
{
#endif

    /**********************
     * GLOBAL PROTOTYPES
     **********************/
    esp_err_t app_camera_init(void);
    esp_err_t app_lvgl_init(void);

#ifdef __cplusplus
}
#endif

#endif // APP_PERIPHERALS_H