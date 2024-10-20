/*********************
 *      INCLUDES
 *********************/
#include "ui/ui_home.h"
#include "stdio.h"
#include "stdlib.h"
#include <string.h>
#include "esp_log.h"
#include "alloc_utils.h"
#include "controller/ctrl_home.h"
#include "ui/ui_qr_code.h"
#include "ui/ui_toast.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "UI_HOME"
#define CANVAS_WIDTH 240 // ref: app_peripherals.h #define CAMERA_FRAME_SIZE FRAMESIZE_240X240
#define CANVAS_HEIGHT 240
#define ITEM_HEIGHT_TITLE 20
#define ITEM_HEIGHT_CONTENT 30
#define ITEM_HEIGHT (ITEM_HEIGHT_TITLE + ITEM_HEIGHT_CONTENT)

/**********************
 *      MACROS
 **********************/
#define NO_BODER_PADDING_STYLE(_object)               \
    do                                                \
    {                                                 \
        lv_obj_set_style_border_width(_object, 0, 0); \
        lv_obj_set_style_pad_all(_object, 0, 0);      \
        lv_obj_set_style_margin_all(_object, 0, 0);   \
    } while (0)

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t *tv = NULL;
static lv_obj_t *qr_dialog = NULL;
static lv_obj_t *preview_image = NULL;
static alloc_utils_memory_struct *alloc_utils_memory_struct_pointer;
static alloc_utils_memory_struct *alloc_utils_memory_struct_pointer_qr_dialog;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void free_current_page();
static void wallet_list_item_event_handler(lv_event_t *e);
static void create_tab_wallet(lv_obj_t *parent);
static void create_tab_scanner(lv_obj_t *parent);
static void create_tab_settings(lv_obj_t *parent);
static void lv_tabview_event_handler(lv_event_t *e);
static void show_connect_qr_code_dialog(ctrl_home_network_data *network_data);
static void close_connect_qr_code_dialog_event_handler(lv_event_t *e);
static void connect_qr_code_choose_group_event_handler(lv_event_t *e);
/**********************
 * GLOBAL PROTOTYPES
 **********************/
void ui_home_create(void);
void ui_home_start_qr_scan(void);
void ui_home_stop_qr_scan(void);

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void wallet_list_item_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        lv_obj_t *clicked_item = lv_event_get_target(e);
        ctrl_home_network_data *network_data = (ctrl_home_network_data *)lv_obj_get_user_data(clicked_item);
        while (network_data == NULL)
        {
            clicked_item = lv_obj_get_parent(clicked_item);
            network_data = (ctrl_home_network_data *)lv_obj_get_user_data(clicked_item);
        }
        if (network_data->compatible_wallet_group == NULL)
        {
            ui_toast_show("The network is not supported yet.", 2000);
        }
        else
        {
            show_connect_qr_code_dialog(network_data);
        }
    }
}
static void create_tab_wallet(lv_obj_t *parent)
{
    lv_obj_set_style_border_width(parent, 0, 0);
    lv_obj_set_style_pad_all(parent, 0, 0);
    lv_obj_set_style_margin_all(parent, 0, 0);
    lv_obj_set_scroll_dir(parent, LV_DIR_NONE);

    int parent_width = lv_obj_get_width(parent);
    lv_obj_t *list = lv_list_create(parent);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_style_margin_all(list, 0, 0);
    lv_obj_set_style_margin_top(list, 3, 0);
    lv_obj_set_style_margin_bottom(list, 3, 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);

    // NO_BODER_PADDING_STYLE(list);
    // lv_obj_set_style_bg_color(list, lv_color_hex(0xff0000), 0);
    lv_obj_set_size(list, parent_width, lv_pct(100));
    lv_obj_center(list);

    ctrl_home_network_data *network_data = ctrl_home_list_networks();
    while (network_data != NULL)
    {
        {
            // lv_obj_t *list_item = lv_list_add_text(list, NULL, NULL);

            /*
            UI:
                ┌───────┌───────────────────────┐
                │       │  Ethereum Address     │
                │  icon │                       │
                │       │  0x111111111111111111 │
                └───────└───────────────────────┘
             */

            /*Column 1: fix width ${item_height}
             *Column 2: 1 unit from the remaining free space*/
            static int32_t col_dsc[] = {ITEM_HEIGHT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

            /*Row 1: fix width ${item_height/2}
             *Row 2: fix width ${item_height/2}*/
            static int32_t row_dsc[] = {ITEM_HEIGHT_TITLE, ITEM_HEIGHT_CONTENT, LV_GRID_TEMPLATE_LAST};

            /*args */
            /*Create a container with grid*/
            lv_obj_t *cont = lv_obj_create(list);
            lv_obj_set_user_data(cont, (void *)network_data);
            NO_BODER_PADDING_STYLE(cont);
            lv_obj_set_style_pad_row(cont, 0, 0);
            lv_obj_set_style_pad_column(cont, 0, 0);

            /*Disable scroll */
            // lv_obj_set_scroll_dir(cont, LV_DIR_NONE);

            // lv_obj_set_style_bg_color(cont, lv_color_hex(0x00ff00), 0);

            lv_obj_set_size(cont, parent_width, ITEM_HEIGHT);
            lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);

            /*icon */
            lv_obj_t *obj = lv_obj_create(cont);
            NO_BODER_PADDING_STYLE(obj);
            lv_obj_set_size(obj, ITEM_HEIGHT, ITEM_HEIGHT);
            lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, 0, 1,
                                 LV_GRID_ALIGN_STRETCH, 0, 2);
            lv_obj_t *label = lv_label_create(obj);
            lv_label_set_text(label, "ICON");

            /*Chain */
            obj = lv_obj_create(cont);
            NO_BODER_PADDING_STYLE(obj);
            lv_obj_set_size(obj, parent_width - ITEM_HEIGHT, LV_SIZE_CONTENT);
            // lv_obj_set_style_bg_color(obj, lv_color_hex(0x0ff000), 0);
            lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_START, 1, 1,
                                 LV_GRID_ALIGN_CENTER, 0, 1);
            label = lv_label_create(obj);
            lv_obj_set_size(label, parent_width - ITEM_HEIGHT, ITEM_HEIGHT_TITLE);
            lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(label, wallet_list_item_event_handler, LV_EVENT_CLICKED, NULL);

            lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
            lv_label_set_text(label, network_data->name);

            /*Address */
            obj = lv_obj_create(cont);
            NO_BODER_PADDING_STYLE(obj);
            lv_obj_set_size(obj, parent_width - ITEM_HEIGHT, LV_SIZE_CONTENT);
            // lv_obj_set_style_bg_color(obj, lv_color_hex(0xcfcfcf), 0);
            lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_START, 1, 1,
                                 LV_GRID_ALIGN_CENTER, 1, 1);
            label = lv_label_create(obj);
            /*Disable scroll */
            lv_obj_set_scroll_dir(label, LV_DIR_NONE);
            lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(label, wallet_list_item_event_handler, LV_EVENT_CLICKED, NULL);
            lv_obj_set_size(label, parent_width - ITEM_HEIGHT, ITEM_HEIGHT_CONTENT);
            lv_obj_set_style_pad_left(label, 5, 0);
            lv_obj_set_style_pad_right(label, 5, 0);
            // auto warp
            lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
            lv_label_set_text(label, network_data->address);
        }
        network_data = (ctrl_home_network_data *)network_data->next;
    }
}
static void create_tab_scanner(lv_obj_t *parent)
{
    lv_obj_set_style_border_width(parent, 0, 0);
    lv_obj_set_style_pad_all(parent, 0, 0);
    lv_obj_set_style_margin_all(parent, 0, 0);

    preview_image = lv_image_create(parent);
    lv_obj_center(preview_image);
    lv_obj_set_size(preview_image, 240, 240);
    // lv_obj_set_style_transform_rotation(preview_image,0, 0);
    lv_image_set_rotation(preview_image, CAMERA_ROTATION * 10);
}
static void create_tab_settings(lv_obj_t *parent)
{
    lv_obj_set_style_border_width(parent, 0, 0);
    lv_obj_set_style_pad_all(parent, 0, 0);
    lv_obj_set_style_margin_all(parent, 0, 0);

    lv_obj_t *list = lv_list_create(parent);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_style_margin_all(list, 0, 0);
    lv_obj_set_style_margin_top(list, 0, 0);
    lv_obj_set_style_margin_bottom(list, 0, 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);

    lv_obj_set_size(list, lv_pct(100), lv_pct(100));
    lv_obj_center(list);

    lv_list_add_text(list, "Lock Now");
    lv_obj_t *btn = lv_list_add_button(list, LV_SYMBOL_CLOSE, "Lock");
    lv_list_add_text(list, "Erase All Data");
    btn = lv_list_add_button(list, LV_SYMBOL_TRASH, "Erase Wallet");
    /*
           UI:
            ┌────────┬───────┐
            │  icon  │  DDL  │
            └────────┴───────┘
            */

    lv_list_add_text(list, "To protect your data, all data will be erased if the PIN is entered incorrectly too many times");
    lv_obj_t *ddlwarp = lv_list_add_button(list, LV_SYMBOL_WARNING, NULL);
    lv_obj_remove_flag(ddlwarp, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *dd = lv_dropdown_create(ddlwarp);
    NO_BODER_PADDING_STYLE(dd);
    lv_dropdown_set_options(dd, "After 2 times\n"
                                "After 3 times\n"
                                "After 4 times\n"
                                "After 5 times\n"
                                "After 6 times\n"
                                "After 7 times\n"
                                "After 8 times\n"
                                "After 9 times\n"
                                "After 10 times");

    // lv_obj_align(dd, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_flex_grow(dd, 1);

    lv_list_add_text(list, "About");
    btn = lv_list_add_button(list, NULL, "Github");
}
static void lv_tabview_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        lv_obj_t *obj = lv_event_get_target(e);
        uint32_t tab_index = lv_tabview_get_tab_active(obj);
        ESP_LOGI(TAG, "Tab index: %" PRIu32, tab_index);
        if (tab_index == 1)
        {
            ctrl_home_scan_qr_start(preview_image);
        }
        else
        {
            ctrl_home_scan_qr_stop();
        }
    }
}

static void connect_qr_code_choose_group_event_handler(lv_event_t *e)
{
    // #TODO
}
static void close_connect_qr_code_dialog_event_handler(lv_event_t *e)
{
    if (alloc_utils_memory_struct_pointer_qr_dialog != NULL)
    {
        ALLOC_UTILS_FREE_MEMORY(alloc_utils_memory_struct_pointer_qr_dialog);
        alloc_utils_memory_struct_pointer_qr_dialog = NULL;
    }
    if (qr_dialog != NULL)
    {
        lv_obj_del(qr_dialog);
        qr_dialog = NULL;
    }
}
static void show_connect_qr_code_dialog(ctrl_home_network_data *network_data)
{
    /*
    UI:
        ┌─────────────────┐
        │  header         │
        ├─────────────────┤
        │      qr         │
        ├─────────────────┤
        │    buttons      │
        └─────────────────┘
 */

    if (qr_dialog != NULL)
    {
        if (lvgl_port_lock(0))
        {
            lv_obj_del(qr_dialog);
            lvgl_port_unlock();
        }
        qr_dialog = NULL;
    }

    ALLOC_UTILS_INIT_MEMORY_STRUCT(alloc_utils_memory_struct_pointer_qr_dialog);

    if (lvgl_port_lock(0))
    {
        lv_obj_t *screen = lv_scr_act();
        /* get parent size */
        int parent_width = lv_obj_get_width(screen);
        int parent_height = lv_obj_get_height(screen);

        int header_height = parent_height * 0.08;
        int buttons_height = parent_height * 0.2;
        int qr_height = parent_height - header_height - buttons_height;

        int32_t *col_dsc;
        ALLOC_UTILS_MALLOC_MEMORY(alloc_utils_memory_struct_pointer_qr_dialog, col_dsc, sizeof(int32_t) * 2);
        col_dsc[0] = parent_width;
        col_dsc[1] = LV_GRID_TEMPLATE_LAST;

        int32_t *row_dsc;
        ALLOC_UTILS_MALLOC_MEMORY(alloc_utils_memory_struct_pointer_qr_dialog, row_dsc, sizeof(int32_t) * 4);
        row_dsc[0] = header_height;
        row_dsc[1] = qr_height;
        row_dsc[2] = buttons_height;
        row_dsc[3] = LV_GRID_TEMPLATE_LAST;

        qr_dialog = lv_obj_create(screen);
        lv_obj_set_scroll_dir(qr_dialog, LV_DIR_NONE);
        lv_obj_set_style_grid_column_dsc_array(qr_dialog, col_dsc, 0);
        lv_obj_set_style_grid_row_dsc_array(qr_dialog, row_dsc, 0);
        lv_obj_set_size(qr_dialog, parent_width, parent_height);
        lv_obj_set_layout(qr_dialog, LV_LAYOUT_GRID);
        lv_obj_set_style_margin_all(qr_dialog, 0, 0);
        lv_obj_set_style_radius(qr_dialog, 0, 0);
        lv_obj_set_style_pad_all(qr_dialog, 0, 0);

        /* header */
        lv_obj_t *header = lv_button_create(qr_dialog);
        lv_obj_t *close_lab = lv_label_create(header);
        lv_label_set_text(close_lab, LV_SYMBOL_CLOSE);
        lv_obj_add_event_cb(header, close_connect_qr_code_dialog_event_handler, LV_EVENT_CLICKED, NULL);
        lv_obj_align(close_lab, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_set_grid_cell(header, LV_GRID_ALIGN_END, 0, 1,
                             LV_GRID_ALIGN_CENTER, 0, 1);

        /* qr */
        lv_obj_t *qr = lv_obj_create(qr_dialog);
        lv_obj_set_style_border_width(qr, 0, 0);
        lv_obj_set_style_radius(qr, 0, 0);
        lv_obj_set_style_outline_width(qr, 0, 0);
        lv_obj_set_scrollbar_mode(qr, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_size(qr, lv_pct(100), lv_pct(100));
        lv_obj_align(qr, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_flex_flow(qr, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_grid_cell(qr, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 1, 1);

        /* buttons */
        lv_obj_t *buttons = lv_obj_create(qr_dialog);

        lv_obj_set_style_border_width(buttons, 0, 0);
        lv_obj_set_style_radius(buttons, 0, 0);
        lv_obj_set_style_outline_width(buttons, 0, 0);
        lv_obj_set_style_pad_all(buttons, 0, 0);
        lv_obj_set_style_margin_all(buttons, 0, 0);

        lv_obj_set_size(buttons, lv_pct(100), lv_pct(100));
        lv_obj_align(buttons, LV_ALIGN_TOP_MID, 0, 5);
        lv_obj_set_flex_align(buttons, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        // lv_obj_set_flex_flow(words, LV_FLEX_FLOW_ROW);
        lv_obj_set_scrollbar_mode(buttons, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_grid_cell(buttons, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 2, 1);
        lv_obj_set_style_pad_left(buttons, 5, 0);
        char *_qr_code = ctrl_home_get_connect_qrcode(network_data, network_data->compatible_wallet_group->qr_type);
        if (_qr_code != NULL)
        {
            char *qr_code = NULL;
            ALLOC_UTILS_MALLOC_MEMORY(alloc_utils_memory_struct_pointer_qr_dialog, qr_code, sizeof(char) * (strlen(_qr_code) + 1));
            strcpy(qr_code, _qr_code);
            free(_qr_code);

            lv_obj_t *qr_canvas = lv_canvas_create(qr);
            lv_obj_center(qr_canvas);
            ui_qr_code_show(qr_canvas, parent_width, qr_code, false);
        }

        lvgl_port_unlock();
    }
}
static void free_current_page()
{
    close_connect_qr_code_dialog_event_handler(NULL);
    if (lvgl_port_lock(0))
    {
        if (tv != NULL)
        {
            lv_obj_del(tv);
            tv = NULL;
        }
        lvgl_port_unlock();
    }
    if (alloc_utils_memory_struct_pointer != NULL)
    {
        ALLOC_UTILS_FREE_MEMORY(alloc_utils_memory_struct_pointer);
    }
}
/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void ui_home_create(void)
{
    // ALLOC_UTILS_INIT_MEMORY_STRUCT(alloc_utils_memory_struct_pointer);
    if (lvgl_port_lock(0))
    {
        tv = lv_tabview_create(lv_scr_act());
        if (tv == NULL)
        {
            ESP_LOGE(TAG, "Failed to create tabview");
            return;
        }
        lv_obj_set_size(tv, lv_pct(100), lv_pct(100));
        lv_tabview_set_tab_bar_position(tv, LV_DIR_BOTTOM);
        lv_tabview_set_tab_bar_size(tv, 50);

        lv_obj_t *tab_wallet = lv_tabview_add_tab(tv, "Wallet");
        if (tab_wallet == NULL)
        {
            ESP_LOGE(TAG, "Failed to create tab");
            return;
        }
        lv_obj_t *tab_scanner = lv_tabview_add_tab(tv, "Scanner");
        if (tab_scanner == NULL)
        {
            ESP_LOGE(TAG, "Failed to create tab");
            return;
        }
        lv_obj_t *tab_settings = lv_tabview_add_tab(tv, "Settings");
        if (tab_settings == NULL)
        {
            ESP_LOGE(TAG, "Failed to create tab");
            return;
        }
        create_tab_wallet(tab_wallet);
        create_tab_scanner(tab_scanner);
        create_tab_settings(tab_settings);
        lv_obj_add_event_cb(tv, lv_tabview_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

        lvgl_port_unlock();
    }
}
void ui_home_start_qr_scan(void)
{
    if (tv != NULL)
    {
        if (lvgl_port_lock(0))
        {
            if (lv_tabview_get_tab_active(tv) != 1)
            {
                lv_tabview_set_active(tv, 1, LV_ANIM_OFF);
            }
            lvgl_port_unlock();
        }
    }
}
void ui_home_stop_qr_scan(void)
{
    if (tv != NULL)
    {
        if (lvgl_port_lock(0))
        {
            if (lv_tabview_get_tab_active(tv) != 0)
            {
                lv_tabview_set_active(tv, 0, LV_ANIM_OFF);
            }
            lvgl_port_unlock();
        }
    }
}