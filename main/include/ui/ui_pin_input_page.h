#ifndef UI_PIN_INPUT_PAGE_H
#define UI_PIN_INPUT_PAGE_H

/*********************
 *      INCLUDES
 *********************/
#include "esp_lvgl_port.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**********************
     *      TYPEDEFS
     **********************/
    typedef char *(*verify_function)(char *);

    /**********************
     * GLOBAL PROTOTYPES
     **********************/
    void ui_create_pin_input_page_set(lv_obj_t *lv_parent, bool show_close_btn);
    void ui_create_pin_input_page_verify(lv_obj_t *lv_parent, bool show_close_btn, char *error_msg, verify_function verify_fun);

#ifdef __cplusplus
    extern "C"
}
#endif

#endif /* UI_PIN_INPUT_PAGE_H */
