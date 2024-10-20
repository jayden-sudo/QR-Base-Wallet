/*********************
 *      INCLUDES
 *********************/
#include "ui/ui_toast.h"
#include "esp_log.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "ui_toast"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void close_toast(lv_timer_t *timer);

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void ui_toast_show(const char *text, int duration);

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void close_toast(lv_timer_t *timer)
{
    lv_obj_t *mask_view = (lv_obj_t *)lv_timer_get_user_data(timer);
    if (lvgl_port_lock(0))
    {
        lv_obj_del(mask_view);
        lvgl_port_unlock();
    }
    lv_timer_del(timer);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void ui_toast_show(const char *text, int duration)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_t *screen = lv_scr_act();
        lv_obj_t *mask_view = lv_obj_create(screen);
        size_t width = lv_obj_get_width(screen);
        size_t label_width = width * 0.7;
        lv_obj_set_size(mask_view, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(mask_view, lv_color_hex(0x111111), 0);
        lv_obj_set_style_bg_opa(mask_view, LV_OPA_90, 0);
        lv_obj_align(mask_view, LV_ALIGN_BOTTOM_MID, 0, -30);
        lv_obj_t *label = lv_label_create(mask_view);
        lv_label_set_text(label, text);
        lv_obj_set_size(label, label_width, LV_SIZE_CONTENT);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
        lv_obj_center(label);

        lv_timer_create(close_toast, duration, mask_view);

        lvgl_port_unlock();
    }
}