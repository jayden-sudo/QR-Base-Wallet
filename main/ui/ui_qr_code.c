/*********************
 *      INCLUDES
 *********************/
#include "ui/ui_qr_code.h"
#include "ui/ui_loading.h"
#include "qrcodegen.h"
#include "esp_log.h"
#include "include/app_peripherals.h"
#include <string.h>

/*********************
 *      DEFINES
 *********************/
#define TAG "UI_QR_CODE"

/**********************
 *      TYPEDEFS
 **********************/
typedef struct
{
    lv_obj_t *canvas;
    int max_width;
    const char *qr_code_str;
    int canvas_size;
    bool sync;
} ui_qr_code_show_t;

/**********************
 *  STATIC VARIABLES
 **********************/
static uint8_t QRCODE_BUFFER[qrcodegen_BUFFER_LEN_MAX];
static uint8_t QRCODE_TEMP_BUFFER[qrcodegen_BUFFER_LEN_MAX];

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void draw_qr_code_task(void *parameters);

/**********************
 * GLOBAL PROTOTYPES
 **********************/
int ui_qr_code_show(lv_obj_t *canvas, int max_width, const char *qr_code_str, bool sync);

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void draw_qr_code_task(void *parameters)
{
    ui_loading_show();
    ui_qr_code_show_t *qr_code_show = (ui_qr_code_show_t *)parameters;
    bool ok = qrcodegen_encodeText(qr_code_show->qr_code_str, QRCODE_TEMP_BUFFER, QRCODE_BUFFER, qrcodegen_Ecc_MEDIUM, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
    if (ok)
    {
        int size = qrcodegen_getSize(QRCODE_BUFFER);
        // LV_COLOR_FORMAT_RGB565
        size_t pixel_mul = qr_code_show->max_width / size;
        size_t pixel_size = 2;
        uint8_t *data = (uint8_t *)malloc(size * size * pixel_size * pixel_mul * pixel_mul);
        memset(data, 0, size * size * pixel_size * pixel_mul * pixel_mul);

        lv_obj_set_size(qr_code_show->canvas, size * pixel_mul, size * pixel_mul);
        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                bool module = qrcodegen_getModule(QRCODE_BUFFER, x, y);
                if (module)
                {
                    {
                        size_t index_y_from = y * pixel_mul;
                        size_t index_y_end = index_y_from + pixel_mul;
                        size_t index_x_from = x * pixel_mul;
                        size_t index_x_end = index_x_from + pixel_mul;
                        for (size_t i = index_y_from; i < index_y_end; i++)
                        {
                            for (size_t j = index_x_from; j < index_x_end; j++)
                            {
                                size_t index = (i * size * pixel_mul + j) * pixel_size;
                                data[index] = 0xff;
                                data[index + 1] = 0xff;
                            }
                        }
                    }
                    // size_t index = (y * size + x) * pixel_size;
                    // data[index] = 0xff;
                    // data[index + 1] = 0xff;
                    vTaskDelay(pdMS_TO_TICKS(1));
                }
                // printf(qrcodegen_getModule(QRCODE_BUFFER, x, y) ? "██" : "  ");
            }
        }
        if (lvgl_port_lock(0))
        {
            lv_canvas_set_buffer(qr_code_show->canvas, data, size * pixel_mul, size * pixel_mul, LV_COLOR_FORMAT_RGB565);
            lvgl_port_unlock();
        }
        free(data);

        qr_code_show->canvas_size = size;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to print QR code");
        qr_code_show->canvas_size = 0;
    }
    if (!qr_code_show->sync)
    {
        free(qr_code_show);
    }
    ui_loading_hide();
    vTaskDelete(NULL);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int ui_qr_code_show(lv_obj_t *canvas, int max_width, const char *qr_code_str, bool sync)
{
    ui_qr_code_show_t *qr_code_show = (ui_qr_code_show_t *)malloc(sizeof(ui_qr_code_show_t));
    qr_code_show->canvas = canvas;
    qr_code_show->max_width = max_width;
    qr_code_show->qr_code_str = qr_code_str;
    qr_code_show->canvas_size = -1;

    xTaskCreatePinnedToCore(draw_qr_code_task, "draw_qr_code_task", 3 * 1024, qr_code_show, 10, NULL, MCU_CORE1);
    if (sync)
    {
        while (qr_code_show->canvas_size == -1)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        int canvas_size = qr_code_show->canvas_size;
        free(qr_code_show);
        return canvas_size;
    }
    return -1;
}