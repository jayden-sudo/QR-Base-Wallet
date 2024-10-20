/*********************
 *      INCLUDES
 *********************/
#include "ui/ui_sign.h"
#include "alloc_utils.h"
#include <string.h>
#include "esp_log.h"
#include "controller/ctrl_sign.h"
#include "ui/ui_qr_code.h"
#include "ui/ui_toast.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "ui_sign"

/**********************
 *  STATIC VARIABLES
 **********************/
static alloc_utils_memory_struct *alloc_utils_memory_struct_pointer;
static lv_obj_t *current_page;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void ui_sign_free(void);
static void close_event_handler(lv_event_t *e);

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void ui_sign_init(void);

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void close_event_handler(lv_event_t *e)
{
    ui_sign_free();
    ctrl_sign_free();
}
static void ui_sign_free(void)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_del(current_page);
        lvgl_port_unlock();
    }
    ALLOC_UTILS_FREE_MEMORY(alloc_utils_memory_struct_pointer);
    current_page = NULL;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void ui_sign_init(void)
{
    ALLOC_UTILS_INIT_MEMORY_STRUCT(alloc_utils_memory_struct_pointer);

    ui_toast_show("Transaction decode not implemented yet.", 3000);

    if (lvgl_port_lock(0))
    {

        {
            /*
               UI:
                   ┌─────────────────┐
                   │  header         │
                   ├─────────────────┤
                   │      qr         │
                   └─────────────────┘

            */

            lv_obj_t *screen = lv_scr_act();

            int parent_width = lv_obj_get_width(screen);
            int parent_height = lv_obj_get_height(screen);

            int header_height = parent_height * 0.08;
            int qr_height = parent_height - header_height;

            int32_t *col_dsc;
            ALLOC_UTILS_MALLOC_MEMORY(alloc_utils_memory_struct_pointer, col_dsc, sizeof(int32_t) * 2);
            col_dsc[0] = parent_width;
            col_dsc[1] = LV_GRID_TEMPLATE_LAST;

            int32_t *row_dsc;
            ALLOC_UTILS_MALLOC_MEMORY(alloc_utils_memory_struct_pointer, row_dsc, sizeof(int32_t) * 3);
            row_dsc[0] = header_height;
            row_dsc[1] = qr_height;
            row_dsc[2] = LV_GRID_TEMPLATE_LAST;

            current_page = lv_obj_create(screen);
            lv_obj_set_scroll_dir(current_page, LV_DIR_NONE);
            lv_obj_set_style_grid_column_dsc_array(current_page, col_dsc, 0);
            lv_obj_set_style_grid_row_dsc_array(current_page, row_dsc, 0);
            lv_obj_set_size(current_page, parent_width, parent_height);
            lv_obj_set_layout(current_page, LV_LAYOUT_GRID);
            lv_obj_set_style_margin_all(current_page, 0, 0);
            lv_obj_set_style_outline_width(current_page, 0, 0);
            lv_obj_set_style_radius(current_page, 0, 0);
            lv_obj_set_style_pad_all(current_page, 0, 0);

            /* header */

            lv_obj_t *header = lv_button_create(current_page);
            lv_obj_t *close_lab = lv_label_create(header);
            lv_label_set_text(close_lab, LV_SYMBOL_CLOSE);
            lv_obj_add_event_cb(header, close_event_handler, LV_EVENT_CLICKED, NULL);
            lv_obj_align(close_lab, LV_ALIGN_RIGHT_MID, 0, 0);
            lv_obj_set_grid_cell(header, LV_GRID_ALIGN_END, 0, 1,
                                 LV_GRID_ALIGN_CENTER, 0, 1);

            /* qr */
            /* qr */
            lv_obj_t *qr = lv_obj_create(current_page);
            lv_obj_set_style_border_width(qr, 0, 0);
            lv_obj_set_style_radius(qr, 0, 0);
            lv_obj_set_style_outline_width(qr, 0, 0);
            lv_obj_set_scrollbar_mode(qr, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_size(qr, lv_pct(100), lv_pct(100));
            lv_obj_align(qr, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_flex_flow(qr, LV_FLEX_FLOW_ROW_WRAP);
            lv_obj_set_grid_cell(qr, LV_GRID_ALIGN_STRETCH, 0, 1,
                                 LV_GRID_ALIGN_STRETCH, 1, 1);

            lv_obj_t *qr_canvas = lv_canvas_create(qr);
            lv_obj_center(qr_canvas);

            char *signature = ctrl_sign_get_signature();
            if (signature == NULL)
            {
                ui_toast_show(current_page, "Failed to get signature.");
            }
            else
            {
                ui_qr_code_show(qr_canvas, parent_width > qr_height ? qr_height : parent_width, signature, false);
            }
        }
        lvgl_port_unlock();
    }
}
