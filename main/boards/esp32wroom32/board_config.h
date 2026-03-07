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
// Grouped physically/logically on GPIO25/26/27
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_26
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_25
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_27

// Microphone I2S group (RX, MASTER) - INMP441/ICS43434 style module
// Grouped physically/logically on GPIO32/33/39
#define AUDIO_I2S_MIC_GPIO_SCK GPIO_NUM_33
#define AUDIO_I2S_MIC_GPIO_WS GPIO_NUM_32
#define AUDIO_I2S_MIC_GPIO_DIN GPIO_NUM_39

// 8-pin SPI LCD group (ST7789 240x240)
// Grouped physically/logically on GPIO16/17/18/21/22/23
#define DISPLAY_SPI_SCK_PIN GPIO_NUM_18
#define DISPLAY_SPI_MOSI_PIN GPIO_NUM_23
#define DISPLAY_SPI_CS_PIN GPIO_NUM_21
#define DISPLAY_DC_PIN GPIO_NUM_22
#define DISPLAY_RST_PIN GPIO_NUM_17
#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_16

// Reserved for future 6-pin SD module (SPI):
// SCK=GPIO18, MOSI=GPIO23 (can share with LCD), MISO=GPIO19, CS=GPIO4

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
