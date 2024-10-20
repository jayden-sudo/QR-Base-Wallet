/*********************
 *      INCLUDES
 *********************/
#include "stdio.h"
#include "stdlib.h"
#include "utility/trezor/bip39_english.h"
#include "string.h"
#include "ui/ui_phrase_input_page.h"
#include "ui/ui_events.h"
#include "alloc_utils.h"
#include "esp_log.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "PHRASE_INPUT_PAGE"
#define BIP39_WORDLIST_LEN ((sizeof(wordlist) / sizeof(wordlist[0])) - 1)
#define START_WITH(prefix, prefix_len, str) (strncmp(str, prefix, prefix_len) == 0)

/**********************
 *  STATIC VARIABLES
 **********************/
static const char *btnm_map[] = {
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "\n",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", "\n",
    "Z", "X", "C", "V", "B", "N", "M", LV_SYMBOL_BACKSPACE, ""};
/* btnm_map index of letters */
static const uint8_t btnm_map_index[] = {
    10, 23, 21, 12, 2, 13, 14, 15, 7, 16, 17, 18, 25, 24, 8, 9, 0, 3, 11, 4, 6, 22, 1, 20, 5, 19};
static const char **phrases;
static size_t phrases_len;
static alloc_utils_memory_struct *alloc_utils_memory_struct_pointer;
static lv_obj_t *parent;
static lv_obj_t *current_page;
static lv_obj_t *content;
static lv_obj_t *keyboard;
static lv_obj_t *words;
static char *current_input;
static int cue_from;
static int cue_to;
static char cue_letter[26];
static size_t cue_letter_len;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void phrase_choose_event_handler(lv_event_t *e);
static void msgbox_confirm_event_handler(lv_event_t *e);
static void msgbox_retry_event_handler(lv_event_t *e);
static void update_keyboard_button();
static void phrase_input_handler(lv_event_t *e);
static void free_current_page();
static void close_event_handler(lv_event_t *e);

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void ui_create_phrase_input_page(lv_obj_t *lv_parent, bool show_close_btn);

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void msgbox_confirm_event_handler(lv_event_t *e)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_t *mbox = lv_event_get_user_data(e);
        lv_msgbox_close(mbox);
        lvgl_port_unlock();
    }
    free_current_page();
}
static void msgbox_retry_event_handler(lv_event_t *e)
{
    if (lvgl_port_lock(0))
    {
        lv_obj_t *mbox = lv_event_get_user_data(e);
        lv_msgbox_close(mbox);
        lvgl_port_unlock();
    }
}
static void phrase_choose_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        if (phrases_len < 24)
        {
            if (lvgl_port_lock(0))
            {
                lv_obj_t *clicked_item = lv_event_get_target(e);
                const char *item_arg = (const char *)lv_obj_get_user_data(clicked_item);
                phrases[phrases_len] = item_arg;
                phrases_len++;
                current_input[0] = '\0';
                {
                    lv_obj_t *obj = lv_button_create(content);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_t *label = lv_label_create(obj);
                    lv_label_set_text_fmt(label, "#%zu %s", phrases_len, item_arg);
                    lv_obj_center(label);

                    // scroll `content` to bottom
                    lv_obj_scroll_to_y(content, 999, LV_ANIM_ON);
                }
                lvgl_port_unlock();
            }
            update_keyboard_button();
        }
        if (phrases_len == 24)
        {
            if (lvgl_port_lock(0))
            {
                lv_obj_t *mbox = lv_msgbox_create(NULL);
                lv_obj_set_size(mbox, lv_obj_get_content_width(parent), LV_SIZE_CONTENT);
                lv_msgbox_add_title(mbox, "Mnemonic phrase");
                char *text = malloc(sizeof(char) * 10 * 24);
                sprintf(text, "#1 %s %s %s\n#4 %s %s %s\n#7 %s %s %s\n#10 %s %s %s\n#13 %s %s %s\n#16 %s %s %s\n#19 %s %s %s\n#22 %s %s %s",
                        phrases[0], phrases[1], phrases[2],
                        phrases[3], phrases[4], phrases[5],
                        phrases[6], phrases[7], phrases[8],
                        phrases[9], phrases[10], phrases[11],
                        phrases[12], phrases[13], phrases[14],
                        phrases[15], phrases[16], phrases[17],
                        phrases[18], phrases[19], phrases[20],
                        phrases[21], phrases[22], phrases[23]);
                lv_msgbox_add_text(mbox, text);
                free(text);
                // lv_msgbox_add_close_button(mbox);
                lv_obj_t *btn;
                btn = lv_msgbox_add_footer_button(mbox, "Confirm");
                lv_obj_add_event_cb(btn, msgbox_confirm_event_handler, LV_EVENT_CLICKED, mbox);
                btn = lv_msgbox_add_footer_button(mbox, "Retry");
                lv_obj_add_event_cb(btn, msgbox_retry_event_handler, LV_EVENT_CLICKED, mbox);
                lvgl_port_unlock();
            }
        }
    }
}
static void update_keyboard_button()
{
    if (lvgl_port_lock(0))
    {
        cue_from = -1;
        cue_to = -1;
        memset(cue_letter, 0, 26);
        cue_letter_len = 0;
        size_t current_input_len = strlen(current_input);
        char next_letter = '\0';
        for (size_t i = 0; i < BIP39_WORDLIST_LEN; i++)
        {
            const char *word = wordlist[i];
            if (START_WITH(current_input, current_input_len, word))
            {
                if (cue_from == -1)
                {
                    cue_from = i;
                }
                if (strlen(word) > current_input_len)
                {
                    next_letter = word[current_input_len];
                    if (cue_letter_len == 0 || cue_letter[cue_letter_len - 1] != next_letter)
                    {
                        cue_letter[cue_letter_len] = next_letter;
                        cue_letter_len++;
                    }
                }
            }
            else if (cue_from != -1)
            {
                cue_to = i - 1;
                break;
            }
        }
        char _index;
        bool _disabled;
        for (size_t i = 0; i < sizeof(btnm_map_index); i++)
        {
            _index = btnm_map_index[i];
            _disabled = true;
            for (size_t j = 0; j < cue_letter_len; j++)
            {
                if (cue_letter[j] == 'a' + i)
                {
                    _disabled = false;
                    break;
                }
            }
            if (_disabled)
            {
                lv_buttonmatrix_set_button_ctrl(keyboard, _index, LV_BUTTONMATRIX_CTRL_DISABLED);
            }
            else
            {
                lv_buttonmatrix_clear_button_ctrl(keyboard, _index, LV_BUTTONMATRIX_CTRL_DISABLED);
            }
        }
        lv_obj_t *child;
        while ((child = lv_obj_get_child(words, 0)))
        {
            lv_obj_del(child);
        }
        if (cue_from >= 0)
        {
            for (size_t i = cue_from;; i++)
            {
                if (i > cue_to || i > cue_from + 5)
                {
                    break;
                }
                lv_obj_t *obj = lv_button_create(words);
                lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                lv_obj_t *label = lv_label_create(obj);
                lv_label_set_text(label, wordlist[i]);
                lv_obj_center(label);
                lv_obj_set_user_data(obj, (void *)wordlist[i]);
                lv_obj_add_event_cb(obj, phrase_choose_event_handler, LV_EVENT_CLICKED, NULL);
            }
        }
        lvgl_port_unlock();
    }
}
static void phrase_input_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED)
    {
        lvgl_port_lock(0);
        uint32_t id = lv_buttonmatrix_get_selected_button(obj);
        lvgl_port_unlock();
        if (id < 26 /* A~Z */)
        {
            lvgl_port_lock(0);
            const char *txt = lv_buttonmatrix_get_button_text(obj, id);
            lvgl_port_unlock();
            char c = tolower(txt[0]);
            current_input[strlen(current_input)] = c;
            current_input[strlen(current_input) + 1] = '\0';
            update_keyboard_button();
        }
        else if (id == 26 /* DEL */)
        {
            if (current_input[0] == '\0')
            {
                if (phrases_len > 0)
                {
                    phrases_len--;
                    if (lvgl_port_lock(0))
                    {
                        // remove last item from `content`
                        size_t child_count = lv_obj_get_child_count(content);
                        if (child_count > 0)
                        {
                            lv_obj_t *child = lv_obj_get_child(content, child_count - 1);
                            lv_obj_del(child);

                            // scroll `content` to bottom
                            lv_obj_scroll_to_y(content, 999, LV_ANIM_ON);
                        }

                        lvgl_port_unlock();
                    }
                }
            }
            else
            {
                current_input[strlen(current_input) - 1] = '\0';
            }
            update_keyboard_button();
        }
    }
}
static void close_event_handler(lv_event_t *e)
{
    phrases_len = 0;
    free_current_page();
}
static void free_current_page()
{
    char *phrase = NULL;
    if (phrases_len == 24)
    {
        phrase = malloc(sizeof(char) * 10 * 24);
        sprintf(phrase, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
                phrases[0], phrases[1], phrases[2],
                phrases[3], phrases[4], phrases[5],
                phrases[6], phrases[7], phrases[8],
                phrases[9], phrases[10], phrases[11],
                phrases[12], phrases[13], phrases[14],
                phrases[15], phrases[16], phrases[17],
                phrases[18], phrases[19], phrases[20],
                phrases[21], phrases[22], phrases[23]);
    }
    if (lvgl_port_lock(0))
    {
        lv_obj_del(current_page);
        lvgl_port_unlock();
    }
    ALLOC_UTILS_FREE_MEMORY(alloc_utils_memory_struct_pointer);

    phrases = NULL;
    current_page = NULL;
    content = NULL;
    keyboard = NULL;
    words = NULL;
    current_input = NULL;
    cue_from = 0;
    cue_to = 0;
    cue_letter_len = 0;

    lv_result_t re = lv_obj_send_event(parent, phrase == NULL ? UI_EVENT_PHRASE_CANCEL : UI_EVENT_PHRASE_CONFIRM, (void *)phrase);
    if (re == LV_RESULT_INVALID)
    {
        printf("lv_obj_send_event failed\n");
        free(phrase);
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void ui_create_phrase_input_page(lv_obj_t *lv_parent, bool show_close_btn)
{
    /*
        UI:
            ┌───────────────────┐
            │     header        │
            ├───────────────────┤
            │                   │
            │     content       │
            │                   │
            │                   │
            ├───────────────────┤
            │ words             │
            ├───────────────────┤
            │     keyboard      │
            └───────────────────┘
     */

    parent = lv_parent;

    ALLOC_UTILS_INIT_MEMORY_STRUCT(alloc_utils_memory_struct_pointer);

    if (lvgl_port_lock(0))
    {
        /* get parent size */
        int parent_width = lv_obj_get_width(parent);
        int parent_height = lv_obj_get_height(parent);

        int header_height = parent_height * 0.08;
        int words_height = parent_height * 0.1;
        int keyboard_height = parent_width * 0.45;

        int32_t *col_dsc;
        ALLOC_UTILS_MALLOC_MEMORY(alloc_utils_memory_struct_pointer, col_dsc, sizeof(int32_t) * 2);
        col_dsc[0] = parent_width;
        col_dsc[1] = LV_GRID_TEMPLATE_LAST;

        int32_t *row_dsc;
        ALLOC_UTILS_MALLOC_MEMORY(alloc_utils_memory_struct_pointer, row_dsc, sizeof(int32_t) * 5);
        row_dsc[0] = header_height;
        row_dsc[1] = LV_GRID_FR(1);
        row_dsc[2] = words_height;
        row_dsc[3] = keyboard_height;
        row_dsc[4] = LV_GRID_TEMPLATE_LAST;

        current_page = lv_obj_create(parent);
        lv_obj_set_scroll_dir(current_page, LV_DIR_NONE);
        lv_obj_set_style_grid_column_dsc_array(current_page, col_dsc, 0);
        lv_obj_set_style_grid_row_dsc_array(current_page, row_dsc, 0);
        lv_obj_set_size(current_page, parent_width, parent_height);
        lv_obj_set_layout(current_page, LV_LAYOUT_GRID);
        // lv_obj_set_style_bg_color(current_page, lv_color_hex(0x00ff00), 0);
        lv_obj_set_style_margin_all(current_page, 0, 0);
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

        /* content */
        content = lv_obj_create(current_page);

        lv_obj_set_style_border_width(content, 0, 0);
        lv_obj_set_style_radius(content, 0, 0);
        lv_obj_set_style_outline_width(content, 0, 0);
        lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
        lv_obj_set_style_pad_bottom(content, 40, 0);

        lv_obj_set_size(content, lv_pct(100), lv_pct(100));
        lv_obj_align(content, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_grid_cell(content, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 1, 1);

        /* words */
        words = lv_obj_create(current_page);

        lv_obj_set_style_border_width(words, 0, 0);
        lv_obj_set_style_radius(words, 0, 0);
        lv_obj_set_style_outline_width(words, 0, 0);
        lv_obj_set_style_pad_all(words, 0, 0);
        lv_obj_set_style_margin_all(words, 0, 0);

        lv_obj_set_size(words, lv_pct(100), lv_pct(100));
        lv_obj_align(words, LV_ALIGN_TOP_MID, 0, 5);
        lv_obj_set_flex_align(words, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        // lv_obj_set_flex_flow(words, LV_FLEX_FLOW_ROW);
        lv_obj_set_scrollbar_mode(words, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_grid_cell(words, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 2, 1);
        lv_obj_set_style_pad_left(words, 5, 0);

        /* keyboard */

        keyboard = lv_btnmatrix_create(current_page);
        lv_btnmatrix_set_map(keyboard, btnm_map);

        lv_obj_set_style_border_width(keyboard, 0, 0);
        lv_obj_set_style_radius(keyboard, 0, 0);
        lv_obj_set_style_outline_width(keyboard, 0, 0);
        lv_obj_set_style_pad_all(keyboard, 0, 0);
        lv_obj_set_style_margin_all(keyboard, 0, 0);

        lv_buttonmatrix_set_button_width(keyboard, 26, 2); /*Make "DEL" *2 wide*/
        lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_grid_cell(keyboard, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 3, 1);

        ALLOC_UTILS_MALLOC_MEMORY(alloc_utils_memory_struct_pointer, phrases, sizeof(char *) * 24);
        phrases_len = 0;
        /*max to 20 letters */
        ALLOC_UTILS_MALLOC_MEMORY(alloc_utils_memory_struct_pointer, current_input, sizeof(char) * 20);
        memset(current_input, 0, sizeof(char) * 20);
        update_keyboard_button();
        lv_obj_add_event_cb(keyboard, phrase_input_handler, LV_EVENT_CLICKED, NULL);

        // test
        if (false)
        {
            // sleep 2s
            vTaskDelay(pdMS_TO_TICKS(2000));
            //  until exhaust file evidence reopen mad stumble beach acquire judge fuel raccoon cram arrange sugar swim cluster exile picture curtain velvet choice surge aware
            phrases[0] = "until";
            phrases[1] = "exhaust";
            phrases[2] = "file";
            phrases[3] = "evidence";
            phrases[4] = "reopen";
            phrases[5] = "mad";
            phrases[6] = "stumble";
            phrases[7] = "beach";
            phrases[8] = "acquire";
            phrases[9] = "judge";
            phrases[10] = "fuel";
            phrases[11] = "raccoon";
            phrases[12] = "cram";
            phrases[13] = "arrange";
            phrases[14] = "sugar";
            phrases[15] = "swim";
            phrases[16] = "cluster";
            phrases[17] = "exile";
            phrases[18] = "picture";
            phrases[19] = "curtain";
            phrases[20] = "velvet";
            phrases[21] = "choice";
            phrases[22] = "surge";
            phrases[23] = "aware";
            phrases_len = 24;
            free_current_page();
        }

        lvgl_port_unlock();
    }
}
