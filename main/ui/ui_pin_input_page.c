/*********************
 *      INCLUDES
 *********************/
#include "ui/ui_pin_input_page.h"
#include "ui/ui_events.h"
#include "alloc_utils.h"
#include <string.h>
#include "esp_log.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "ui_pin_input_page"
/**********************
 *  STATIC VARIABLES
 **********************/
static const char *btnm_map[] = {"1", "2", "3", "\n",
                                 "4", "5", "6", "\n",
                                 "7", "8", "9", "\n",
                                 "-", "0", LV_SYMBOL_BACKSPACE, ""};

static alloc_utils_memory_struct *alloc_utils_memory_struct_pointer;
static lv_obj_t *parent;
static lv_obj_t *current_page;
static lv_obj_t *lv_msg;
static lv_obj_t **leds;
static verify_function verify_pin;

static char pin[7];
static char pin_tmp[7];
/**
 * 0: enter new pin
 * 1: reenter new pin
 * 2: verify pin
 */
static int pin_step;

static char *MSG_ENTER_NEW_PIN = "Enter new passcode";
static char *MSG_RE_ENTER_PIN = "Re-Enter passcode";
static char *MSG_VERIFY_PIN = "Enter passcode";

static char *fixed_msg;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void set_led(int num);
static void set_msg(char *msg, bool is_error);
static void set_error_msg(char *msg);
static void set_fixed_msg(char *msg);
static void load_fixed_msg();
static void create_pin_input_page(lv_obj_t *lv_parent, bool show_close_btn);
static void close_event_handler(lv_event_t *e);
static void pin_input_event_handler(lv_event_t *e);
static void pin_input_event_handler(lv_event_t *e);
static void free_current_page();

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void ui_create_pin_input_page_set(lv_obj_t *lv_parent, bool show_close_btn);
void ui_create_pin_input_page_verify(lv_obj_t *lv_parent, bool show_close_btn, char *error_msg, verify_function verify_fun);

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void set_led(int num)
{
    if (num < 0 || num > 6)
    {
        return;
    }
    if (lvgl_port_lock(0))
    {
        for (int i = 0; i < 6; i++)
        {
            lv_obj_t *led = leds[i];
            if (num > i)
            {
                lv_obj_set_style_border_color(led, lv_palette_main(LV_PALETTE_BLUE), 0);
                lv_obj_set_style_bg_color(led, lv_palette_main(LV_PALETTE_BLUE), 0);
            }
            else
            {
                lv_obj_set_style_border_color(led, lv_color_hex(0xcccccc), 0);
                lv_obj_set_style_bg_color(led, lv_color_hex(0xffffff), 0);
            }
        }

        lvgl_port_unlock();
    }
}
static void set_msg(char *msg, bool is_error)
{
    if (lvgl_port_lock(0))
    {

        if (lv_msg == NULL)
        {
            return;
        }
        if (is_error)
        {
            lv_obj_set_style_text_color(lv_msg, lv_color_hex(0xff0000), 0);
        }
        else
        {
            lv_obj_set_style_text_color(lv_msg, lv_color_hex(0x000000), 0);
        }
        lv_label_set_text(lv_msg, msg);

        lvgl_port_unlock();
    }
}
static void set_error_msg(char *msg)
{
    set_msg(msg, true);
}
static void set_fixed_msg(char *msg)
{
    fixed_msg = msg;
    set_msg(msg, false);
}
static void load_fixed_msg()
{
    if (fixed_msg != NULL)
    {
        set_msg(fixed_msg, false);
    }
    else
    {
        set_msg("", false);
    }
}
static void create_pin_input_page(lv_obj_t *lv_parent, bool show_close_btn)
{

    /*
        UI:
            ┌─────────────────┐
            │  header         │
            ├─────────────────┤
            │     msg         │
            ├─────────────────┤
            │   ┌─┐┌─┐┌─┐┌─┐  │
            │   └─┘└─┘└─┘└─┘  │
            ├─────────────────┤
            │     keyboard    │
            └─────────────────┘

     */
    memset(pin, 0, sizeof(pin));
    memset(pin_tmp, 0, sizeof(pin_tmp));

    parent = lv_parent;

    /* get parent size */
    int parent_width = lv_obj_get_width(parent);
    int parent_height = lv_obj_get_height(parent);

    int header_height = parent_height * 0.08;
    int msg_height = parent_height * 0.15;
    int keyboard_height = parent_height * 0.62;

    int32_t *col_dsc;
    ALLOC_UTILS_MALLOC_MEMORY(alloc_utils_memory_struct_pointer, col_dsc, sizeof(int32_t) * 2);
    col_dsc[0] = parent_width;
    col_dsc[1] = LV_GRID_TEMPLATE_LAST;

    int32_t *row_dsc;
    ALLOC_UTILS_MALLOC_MEMORY(alloc_utils_memory_struct_pointer, row_dsc, sizeof(int32_t) * 5);
    row_dsc[0] = header_height;
    row_dsc[1] = LV_GRID_FR(1);
    row_dsc[2] = msg_height;
    row_dsc[3] = keyboard_height;
    row_dsc[4] = LV_GRID_TEMPLATE_LAST;

    current_page = lv_obj_create(parent);
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
    if (show_close_btn)
    {
        lv_obj_t *header = lv_button_create(current_page);
        lv_obj_t *close_lab = lv_label_create(header);
        lv_label_set_text(close_lab, LV_SYMBOL_CLOSE);
        lv_obj_add_event_cb(header, close_event_handler, LV_EVENT_CLICKED, NULL);
        lv_obj_align(close_lab, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_set_grid_cell(header, LV_GRID_ALIGN_END, 0, 1,
                             LV_GRID_ALIGN_CENTER, 0, 1);
    }

    /* msg */
    lv_msg = lv_label_create(current_page);
    lv_obj_set_size(lv_msg, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(lv_msg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_grid_cell(lv_msg, LV_GRID_ALIGN_CENTER, 0, 1,
                         LV_GRID_ALIGN_CENTER, 1, 1);

    /* input circles */
    int32_t *col_circle_dsc;
    ALLOC_UTILS_MALLOC_MEMORY(alloc_utils_memory_struct_pointer, col_circle_dsc, sizeof(int32_t) * 9);
    col_circle_dsc[0] = parent_width / 6;
    col_circle_dsc[1] = parent_width / 9; //   ─┐
    col_circle_dsc[2] = parent_width / 9; //    │
    col_circle_dsc[3] = parent_width / 9; //    ├─ 6 LED
    col_circle_dsc[4] = parent_width / 9; //    │
    col_circle_dsc[5] = parent_width / 9; //    │
    col_circle_dsc[6] = parent_width / 9; //   ─┘
    col_circle_dsc[7] = parent_width / 6;
    col_circle_dsc[8] = LV_GRID_TEMPLATE_LAST;

    int32_t *row_circle_dsc;
    ALLOC_UTILS_MALLOC_MEMORY(alloc_utils_memory_struct_pointer, row_circle_dsc, sizeof(int32_t) * 2);
    row_circle_dsc[0] = LV_GRID_FR(1);
    row_circle_dsc[1] = LV_GRID_TEMPLATE_LAST;

    lv_obj_t *input_circles = lv_obj_create(current_page);
    lv_obj_set_style_grid_column_dsc_array(input_circles, col_circle_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(input_circles, row_circle_dsc, 0);
    lv_obj_set_layout(input_circles, LV_LAYOUT_GRID);
    lv_obj_set_style_pad_column(input_circles, 0, 0);
    lv_obj_set_size(input_circles, parent_width, LV_SIZE_CONTENT);
    lv_obj_set_style_border_width(input_circles, 0, 0);
    lv_obj_set_style_radius(input_circles, 0, 0);
    lv_obj_set_style_pad_all(input_circles, 0, 0);
    lv_obj_set_style_margin_all(input_circles, 0, 0);

    /* 6 LED */
    {

        ALLOC_UTILS_MALLOC_MEMORY(alloc_utils_memory_struct_pointer, leds, sizeof(void *) * 6);

        lv_obj_t *led;
        int led_size = (parent_width / 9) * 0.8;
        for (int i = 0; i < 6; i++)
        {
            led = lv_obj_create(input_circles);
            lv_obj_set_size(led, led_size, led_size);
            lv_obj_set_style_radius(led, led_size, 0);
            lv_obj_set_style_pad_all(led, 0, 0);
            lv_obj_set_style_margin_all(led, 0, 0);
            lv_obj_set_style_border_color(led, lv_color_hex(0xcccccc), 0);
            lv_obj_set_style_bg_color(led, lv_color_hex(0xffffff), 0);
            lv_obj_set_grid_cell(led, LV_GRID_ALIGN_CENTER, i + 1, 1,
                                 LV_GRID_ALIGN_CENTER, 0, 1);
            leds[i] = led;
        }
    }

    lv_obj_align(input_circles, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_grid_cell(input_circles, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 2, 1);

    /* keyboard */
    lv_obj_t *btnm = lv_buttonmatrix_create(current_page);

    lv_obj_set_style_border_width(btnm, 0, 0);
    lv_obj_set_style_radius(btnm, 0, 0);
    lv_obj_set_style_outline_width(btnm, 0, 0);
    lv_obj_set_style_pad_all(btnm, 0, 0);
    lv_obj_set_style_margin_all(btnm, 0, 0);

    // lv_obj_set_style_bg_color(btnm, lv_color_hex(0xf000f0), 0);
    lv_buttonmatrix_set_map(btnm, btnm_map);
    lv_buttonmatrix_set_button_ctrl(btnm, 9, LV_BUTTONMATRIX_CTRL_HIDDEN);
    lv_obj_align(btnm, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_grid_cell(btnm, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 3, 1);

    lv_obj_add_event_cb(btnm, pin_input_event_handler, LV_EVENT_CLICKED, NULL);
}
static void close_event_handler(lv_event_t *e)
{
    pin[0] = '\0';
    free_current_page();
}
static void pin_input_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED)
    {
        load_fixed_msg();

        uint32_t id = lv_buttonmatrix_get_selected_button(obj);
        if (id == 11)
        {
            // backspace
            if (strlen(pin) > 0)
            {
                pin[strlen(pin) - 1] = '\0';
            }
        }
        else
        {
            char pin_num;
            if (id <= 8)
            {
                pin_num = '0' + (id + 1);
            }
            else if (id == 10)
            {
                pin_num = '0';
            }
            else
            {
                /* skip */
                return;
            }

            if (strlen(pin) < 6)
            {
                pin[strlen(pin)] = pin_num;
                pin[strlen(pin)] = '\0';
            }

            if (strlen(pin) == 6)
            {
                if (pin_step == 0)
                {
                    memcpy(pin_tmp, pin, sizeof(pin));
                    pin_step = 1;
                    set_fixed_msg(MSG_RE_ENTER_PIN);
                }
                else if (pin_step == 1)
                {
                    if (strcmp(pin, pin_tmp) == 0)
                    {
                        free_current_page();
                        return;
                    }
                    else
                    {
                        pin_step = 0;
                        set_fixed_msg(MSG_ENTER_NEW_PIN);
                        set_error_msg("Passcode not match!");
                    }
                }
                else if (pin_step == 2)
                {
                    char *result = verify_pin(pin);
                    if (result == NULL)
                    {
                        free_current_page();
                        return;
                    }
                    else
                    {
                        ESP_LOGE(TAG, "verify_pin failed: %s", result);
                        set_error_msg(result);
                    }
                }
                else
                {
                    printf("pin input event handler error\n");
                    exit(0);
                }

                memset(pin, 0, sizeof(pin));
            }
        }
        set_led(strlen(pin));
    }
}
static void free_current_page()
{
    char *pin_str = NULL;
    if (pin[0] != '\0')
    {
        pin_str = malloc(sizeof(char) * 7);
        sprintf(pin_str, "%s", pin);
    }

    {
        if (lvgl_port_lock(0))
        {
            lv_obj_del(current_page);
            lvgl_port_unlock();
        }

        ALLOC_UTILS_FREE_MEMORY(alloc_utils_memory_struct_pointer);

        current_page = NULL;
        lv_msg = NULL;
        leds = NULL;
    }

    lv_result_t re = lv_obj_send_event(parent,
                                       pin_str == NULL ? UI_EVENT_PIN_CANCEL : UI_EVENT_PIN_CONFIRM,
                                       (void *)pin_str);
    if (re == LV_RESULT_INVALID)
    {
        printf("lv_obj_send_event failed\n");
        free(pin_str);
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void ui_create_pin_input_page_set(lv_obj_t *lv_parent, bool show_close_btn)
{
    if (lvgl_port_lock(0))
    {
        ALLOC_UTILS_INIT_MEMORY_STRUCT(alloc_utils_memory_struct_pointer);

        pin_step = 0;
        create_pin_input_page(lv_parent, show_close_btn);
        set_fixed_msg(MSG_ENTER_NEW_PIN);

        lvgl_port_unlock();
    }
}
void ui_create_pin_input_page_verify(lv_obj_t *lv_parent, bool show_close_btn, char *error_msg, verify_function verify_fun)
{
    if (lvgl_port_lock(0))
    {

        ALLOC_UTILS_INIT_MEMORY_STRUCT(alloc_utils_memory_struct_pointer);

        pin_step = 2;
        verify_pin = verify_fun;

        create_pin_input_page(lv_parent, show_close_btn);

        set_fixed_msg(MSG_VERIFY_PIN);
        if (error_msg != NULL)
        {
            char *_msg = NULL;
            ALLOC_UTILS_MALLOC_MEMORY(alloc_utils_memory_struct_pointer, _msg, sizeof(char) * (strlen(error_msg) + 1));
            sprintf(_msg, "%s", error_msg);
            set_error_msg(_msg);
        }

        lvgl_port_unlock();
    }
}
