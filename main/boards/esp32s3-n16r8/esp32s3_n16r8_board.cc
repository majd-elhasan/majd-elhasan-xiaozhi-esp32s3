#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/lcd_display.h"
#include "system_reset.h"
#include "application.h"
#include "button.h"
#include "assets/lang_config.h"
#include "led/single_led.h"
#include "quran_player.h"

#include "board_config.h"
#include <wifi_station.h>

#include <esp_log.h>
#include <driver/spi_master.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>

#define TAG "Esp32S3N16R8Board"

class Esp32S3N16R8Board : public WifiBoard {
private:
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    Display* display_ = nullptr;

    Button boot_button_;
    Button touch_button_;
    Button volume_up_button_;
    Button volume_down_button_;

    void InitDisplaySpi() {
        // Keep SD card deselected while we only drive the LCD on the shared SPI bus.
        gpio_config_t sd_cs_gpio_config = {
            .pin_bit_mask = 1ULL << SD_SPI_CS_PIN,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&sd_cs_gpio_config));
        ESP_ERROR_CHECK(gpio_set_level(SD_SPI_CS_PIN, 1));

        spi_bus_config_t bus_cfg = {};
        bus_cfg.mosi_io_num = DISPLAY_SPI_MOSI_PIN;
        bus_cfg.miso_io_num = DISPLAY_SPI_MISO_PIN;
        bus_cfg.sclk_io_num = DISPLAY_SPI_SCK_PIN;
        bus_cfg.quadwp_io_num = GPIO_NUM_NC;
        bus_cfg.quadhd_io_num = GPIO_NUM_NC;
        bus_cfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);

        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));
    }

    void InitializeSt7789Display() {
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_SPI_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = 0;
        io_config.pclk_hz = DISPLAY_SPI_PCLK_HZ;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;

        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &panel_io_));

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RST_PIN;
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.bits_per_pixel = 16;

        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io_, &panel_config, &panel_));
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel_));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_, DISPLAY_COLOR_INVERT));
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_, true));

        if (DISPLAY_BACKLIGHT_PIN != GPIO_NUM_NC) {
            gpio_config_t bk_gpio_config = {
                .pin_bit_mask = 1ULL << DISPLAY_BACKLIGHT_PIN,
                .mode = GPIO_MODE_OUTPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE,
            };
            ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
            ESP_ERROR_CHECK(gpio_set_level(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT ? 0 : 1));
        }

        display_ = new SpiLcdDisplay(
            panel_io_,
            panel_,
            DISPLAY_WIDTH,
            DISPLAY_HEIGHT,
            DISPLAY_OFFSET_X,
            DISPLAY_OFFSET_Y,
            DISPLAY_MIRROR_X,
            DISPLAY_MIRROR_Y,
            DISPLAY_SWAP_XY
        );
    }

    void InitButtons() {
        auto enter_ap_mode = [this]() {
            ESP_LOGI(TAG, "Boot button trigger => entering WiFi AP config mode");
            EnterWifiConfigMode();
        };

        boot_button_.OnClick([this]() {
            Application::GetInstance().ToggleChatState();
        });
        boot_button_.OnMultipleClick(enter_ap_mode, 7);
        boot_button_.OnLongPress(enter_ap_mode);
    }

public:
    Esp32S3N16R8Board() :
        boot_button_(BOOT_BUTTON_GPIO, false, 7000 /* long press ms */),
        touch_button_(TOUCH_BUTTON_GPIO),
        volume_up_button_(VOLUME_UP_BUTTON_GPIO),
        volume_down_button_(VOLUME_DOWN_BUTTON_GPIO) {

        ESP_LOGI(TAG, "Init Esp32Wroom32Board");
        InitDisplaySpi();
        InitializeSt7789Display();
        QuranPlayer::GetInstance().Initialize(
            SPI2_HOST,
            DISPLAY_SPI_SCK_PIN,
            DISPLAY_SPI_MOSI_PIN,
            DISPLAY_SPI_MISO_PIN,
            SD_SPI_CS_PIN,
            DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t));
        InitButtons();
    }

    virtual Led* GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

    virtual AudioCodec* GetAudioCodec() override {
#ifndef AUDIO_I2S_METHOD_SIMPLEX
        static NoAudioCodecDuplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK,
            AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT,
            AUDIO_I2S_GPIO_DIN
        );
#else
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK,
            AUDIO_I2S_SPK_GPIO_LRCK,
            AUDIO_I2S_SPK_GPIO_DOUT,
            AUDIO_I2S_MIC_GPIO_SCK,
            AUDIO_I2S_MIC_GPIO_WS,
            AUDIO_I2S_MIC_GPIO_DIN
        );
#endif
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }
};

DECLARE_BOARD(Esp32S3N16R8Board);
