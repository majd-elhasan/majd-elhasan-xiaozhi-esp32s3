#ifndef BOARD_CONFIG_H_
#define BOARD_CONFIG_H_

#include <driver/gpio.h>

// Audio rate
#define AUDIO_INPUT_REFERENCE true
#define AUDIO_INPUT_SAMPLE_RATE 16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// Enable Simplex (two bus systems)
#define AUDIO_I2S_METHOD_SIMPLEX

// Speaker I2S group (TX, MASTER) - MAX98357 style module
// ESP32-S3 mapping
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_16
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_17
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_18

// Microphone I2S group (RX, MASTER) - INMP441/ICS43434 style module
// ESP32-S3 mapping
#define AUDIO_I2S_MIC_GPIO_SCK GPIO_NUM_4
#define AUDIO_I2S_MIC_GPIO_WS GPIO_NUM_5
#define AUDIO_I2S_MIC_GPIO_DIN GPIO_NUM_6

// Shared SPI bus for ST7789 LCD + microSD
#define DISPLAY_SPI_SCK_PIN GPIO_NUM_12
#define DISPLAY_SPI_MOSI_PIN GPIO_NUM_11
#define DISPLAY_SPI_MISO_PIN GPIO_NUM_13
#define DISPLAY_SPI_CS_PIN GPIO_NUM_10
#define DISPLAY_DC_PIN GPIO_NUM_9
#define DISPLAY_RST_PIN GPIO_NUM_8
#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_7
#define SD_SPI_CS_PIN GPIO_NUM_14

#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 240

#define DISPLAY_OFFSET_X 0
#define DISPLAY_OFFSET_Y 0
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY false
#define DISPLAY_COLOR_INVERT true
#define DISPLAY_SPI_PCLK_HZ (40 * 1000 * 1000)
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false

// Onboard LED
#define BUILTIN_LED_GPIO GPIO_NUM_2

// Buttons
#define BOOT_BUTTON_GPIO GPIO_NUM_0
// Optional volume buttons are not wired on this board variant.
// Keep them disabled to avoid internal pull-up errors on input-only pins.
#define VOLUME_UP_BUTTON_GPIO GPIO_NUM_NC
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_NC

// Reuse BOOT key for press-to-talk by default.
#define TOUCH_BUTTON_GPIO BOOT_BUTTON_GPIO

#endif  // BOARD_CONFIG_H_
