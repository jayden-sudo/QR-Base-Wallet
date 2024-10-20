#ifndef PERIPHERALS_H
#define PERIPHERALS_H

/*********************
 * Camera Module Settings
 *********************/
#define CONFIG_CAMERA_MODULE_WROVER_KIT 0
#define CONFIG_CAMERA_MODULE_ESP_EYE 0
#define CONFIG_CAMERA_MODULE_ESP_S2_KALUGA 0
#define CONFIG_CAMERA_MODULE_ESP_S3_EYE 1
#define CONFIG_CAMERA_MODULE_ESP32_CAM_BOARD 0
#define CONFIG_CAMERA_MODULE_M5STACK_PSRAM 0
#define CONFIG_CAMERA_MODULE_M5STACK_WIDE 0
#define CONFIG_CAMERA_MODULE_AI_THINKER 0
#define CONFIG_CAMERA_MODULE_CUSTOM 0

#define XCLK_FREQ_HZ 15000000

// NOTE: only support PIXFORMAT_RGB565 yet
#define CAMERA_PIXFORMAT PIXFORMAT_RGB565
// NOTE: only support FRAMESIZE_240X240 yet
#define CAMERA_FRAME_SIZE FRAMESIZE_240X240

#define CAMERA_FB_COUNT 2

#define CAMERA_ROTATION 180

/*********************
 * LCD Module Settings
 *********************/
// set lcd module here
#define CONFIG_LCD_ST7735 0
#define CONFIG_LCD_ST7789 1
#define CONFIG_LCD_ST7796 0
#define CONFIG_LCD_ILI9341 0
#define CONFIG_LCD_GC9A01 0
// set lcd gpio pins here
#define CONFIG_LCD_GPIO_SCLK (39)
#define CONFIG_LCD_GPIO_MOSI (40)
#define CONFIG_LCD_GPIO_RST (41)
#define CONFIG_LCD_GPIO_DC (42)
#define CONFIG_LCD_GPIO_CS (2)
#define CONFIG_LCD_GPIO_BL (1)
// set lcd resolution here
#define CONFIG_LCD_H_RES (240)
#define CONFIG_LCD_V_RES (320)
// set lcd mirror here
#define CONFIG_LCD_SWAP_XY 0
#define CONFIG_LCD_MIRROR_X 0
#define CONFIG_LCD_MIRROR_Y 0
// set lcd other settings here
#define CONFIG_LCD_SPI_NUM (SPI3_HOST)
#define CONFIG_LCD_PIXEL_CLK_HZ (40 * 1000 * 1000)
#define CONFIG_LCD_CMD_BITS (8)
#define CONFIG_LCD_PARAM_BITS (8)
#define CONFIG_LCD_COLOR_SPACE (ESP_LCD_COLOR_SPACE_BGR)
#define CONFIG_LCD_BITS_PER_PIXEL (16)
#define CONFIG_LCD_DRAW_BUFF_DOUBLE (1)
#define CONFIG_LCD_DRAW_BUFF_HEIGHT (50)
#define CONFIG_LCD_BL_ON_LEVEL (1)

/*********************
 * Touch Module Settings
 *********************/
// Set touch module here
#define CONFIG_TOUCH_CST816S 0
#define CONFIG_TOUCH_FT5X06 1
#define CONFIG_TOUCH_GT1151 0
#define CONFIG_TOUCH_GT911 0
#define CONFIG_TOUCH_TT21100 0
// set touch mirror here
#define CONFIG_TOUCH_SWAP_XY 0
#define CONFIG_TOUCH_MIRROR_X 0
#define CONFIG_TOUCH_MIRROR_Y 0
// Touch settings
#define CONFIG_TOUCH_I2C_NUM (0)
#define CONFIG_TOUCH_I2C_CLK_HZ (400000)
// Touch pins
#define CONFIG_TOUCH_I2C_SCL (21)
#define CONFIG_TOUCH_I2C_SDA (20)
#define CONFIG_TOUCH_GPIO_INT (19)

#endif // PERIPHERALS_H