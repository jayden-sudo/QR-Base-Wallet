#ifndef UI_PHRASE_INPUT_PAGE_H
#define UI_PHRASE_INPUT_PAGE_H

/*********************
 *      INCLUDES
 *********************/
#include "ui_events.h"
#include "esp_lvgl_port.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**********************
     * GLOBAL PROTOTYPES
     **********************/
    void ui_create_phrase_input_page(lv_obj_t *lv_parent, bool show_close_btn);

#ifdef __cplusplus
    extern "C"
}
#endif

#endif /* UI_PHRASE_INPUT_PAGE_H */