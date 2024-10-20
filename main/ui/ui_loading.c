/*********************
 *      INCLUDES
 *********************/
#include "ui/ui_loading.h"
#include "esp_log.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "ui_loading"

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t *mask_view = NULL;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void ui_loading_show(void);
void ui_loading_hide(void);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void ui_loading_show(void)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_t *screen = lv_scr_act();
        mask_view = lv_obj_create(screen);
        lv_obj_set_size(mask_view, lv_obj_get_width(screen), lv_obj_get_height(screen));
        lv_obj_set_style_bg_color(mask_view, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_bg_opa(mask_view, LV_OPA_70, 0);

        lv_obj_t *spinner = lv_spinner_create(mask_view);
        lv_obj_set_size(spinner, 100, 100);
        lv_obj_center(spinner);
        lv_spinner_set_anim_params(spinner, 10000, 200);
        lvgl_port_unlock();
    }
}
void ui_loading_hide(void)
{
    if (mask_view != NULL)
    {
        if (lvgl_port_lock(0))
        {
            lv_obj_del(mask_view);
            mask_view = NULL;
            lvgl_port_unlock();
        }
    }
}