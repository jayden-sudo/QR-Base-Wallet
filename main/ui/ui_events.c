/*********************
 *      INCLUDES
 *********************/
#include "ui/ui_events.h"
#include "esp_lvgl_port.h"

/**********************
 *      VARIABLES
 **********************/
uint32_t UI_EVENT_PHRASE_CONFIRM = 0;
uint32_t UI_EVENT_PHRASE_CANCEL = 0;
uint32_t UI_EVENT_PIN_CONFIRM = 0;
uint32_t UI_EVENT_PIN_CANCEL = 0;

/**********************
 *  STATIC VARIABLES
 **********************/
static bool ui_events_inited = false;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void ui_init_events(void);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void ui_init_events(void)
{
    if (ui_events_inited)
        return;
    ui_events_inited = true;
    UI_EVENT_PHRASE_CONFIRM = lv_event_register_id();
    UI_EVENT_PHRASE_CANCEL = lv_event_register_id();
    UI_EVENT_PIN_CONFIRM = lv_event_register_id();
    UI_EVENT_PIN_CANCEL = lv_event_register_id();
}
