#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include "lvgl_display.h"
#include "gif/lvgl_gif.h"
#include "application.h"
#include "device_state.h"
#include "lvgl_theme.h"

#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <font_emoji.h>
#include <font_awesome.h>
#include <ctime>
#include <string>

#include <atomic>
#include <memory>

#ifdef __cplusplus
extern "C" {
#endif
extern const lv_font_t font_noto_basic_30_4;
#ifdef __cplusplus
}
#endif

#define PREVIEW_IMAGE_DURATION_MS 5000


class LcdDisplay : public LvglDisplay {
protected:
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    
    lv_draw_buf_t draw_buf_;
    lv_obj_t* top_bar_ = nullptr;
    lv_obj_t* status_bar_ = nullptr;
    lv_obj_t* content_ = nullptr;
    lv_obj_t* container_ = nullptr;
    lv_obj_t* side_bar_ = nullptr;
    lv_obj_t* bottom_bar_ = nullptr;
    lv_obj_t* preview_image_ = nullptr;
    lv_obj_t* emoji_label_ = nullptr;
    lv_obj_t* emoji_image_ = nullptr;
    std::unique_ptr<LvglGif> gif_controller_ = nullptr;
    lv_obj_t* emoji_box_ = nullptr;
    lv_obj_t* chat_message_label_ = nullptr;
    esp_timer_handle_t preview_timer_ = nullptr;
    std::unique_ptr<LvglImage> preview_image_cached_ = nullptr;
    bool hide_subtitle_ = false;  // Control whether to hide chat messages/subtitles
    std::string current_emoji_utf8_ = FONT_AWESOME_MICROCHIP_AI;
    const lv_font_t* current_emoji_font_ = nullptr;

    void InitializeLcdThemes();
    void SetupUI();
    virtual bool Lock(int timeout_ms = 0) override;
    virtual void Unlock() override;

protected:
    // Add protected constructor
    LcdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width, int height);
    
public:
    ~LcdDisplay();
    virtual void SetEmotion(const char* emotion) override;
    virtual void SetChatMessage(const char* role, const char* content) override;
    virtual void ClearChatMessages() override;
    virtual void SetPreviewImage(std::unique_ptr<LvglImage> image) override;
    virtual void UpdateStatusBar(bool update_all = false) override {
        DisplayLockGuard guard(this);  // ensure LVGL thread safety

        // Run base status updates (icons, battery, status text/time)
        LvglDisplay::UpdateStatusBar(update_all);

        if (emoji_box_ == nullptr || emoji_label_ == nullptr) {
            return;
        }

        auto state = Application::GetInstance().GetDeviceState();
        if (state == kDeviceStateIdle) {
            // Use the centered emoji label as a big clock during idle
            time_t now = time(nullptr);
            struct tm* tm = localtime(&now);
            static int last_minute = -1;
            if (tm && tm->tm_year >= 125) {  // year >= 2025
                if (tm->tm_min != last_minute || update_all) {
                    last_minute = tm->tm_min;
                    char time_str[16];
                    strftime(time_str, sizeof(time_str), "%H:%M", tm);
                    // Developer option
                    #define EMOJI_ZOOM_150 1   // set to 1 for 150%, set to 0 for normal size
                    // Use an existing large text font (30pt) and scale it up to fill the center
                    lv_obj_set_style_text_font(emoji_label_, &font_noto_basic_30_4, 0);
                    #if EMOJI_ZOOM_150
                        // Center the zoom pivot so scaling grows symmetrically
                        int pivot_x = lv_obj_get_width(emoji_label_) / 2;
                        int pivot_y = lv_obj_get_height(emoji_label_) / 2;
                        lv_obj_set_style_transform_pivot_x(emoji_label_, pivot_x, 0);
                        lv_obj_set_style_transform_pivot_y(emoji_label_, pivot_y, 0);
                        lv_obj_set_style_transform_zoom(emoji_label_, 384, 0);  // 150% zoom
                    #else // normal size (100% zoom)
                        lv_obj_set_style_text_font(emoji_label_, &font_noto_basic_30_4, 0);
                        lv_obj_set_style_transform_zoom(emoji_label_, 256, 0);  // 100% (no zoom)
                    #endif
                    lv_label_set_text(emoji_label_, time_str);
                }
            }
            lv_obj_remove_flag(emoji_box_, LV_OBJ_FLAG_HIDDEN);
            if (emoji_image_ != nullptr) {
                lv_obj_add_flag(emoji_image_, LV_OBJ_FLAG_HIDDEN);
            }
            if (status_label_ != nullptr) {
                lv_obj_add_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
            }
            lv_obj_set_style_text_align(emoji_label_, LV_TEXT_ALIGN_CENTER, 0);
            lv_label_set_long_mode(emoji_label_, LV_LABEL_LONG_CLIP);
        } else {
            // Restore normal center icon + status text when not idle
            if (!current_emoji_utf8_.empty()) {
                if (current_emoji_font_ != nullptr) {
                    lv_obj_set_style_text_font(emoji_label_, current_emoji_font_, 0);
                }
                lv_label_set_text(emoji_label_, current_emoji_utf8_.c_str());
            }
            lv_obj_set_style_transform_zoom(emoji_label_, 256, 0);  // reset to 100%
            lv_obj_set_style_transform_pivot_x(emoji_label_, 0, 0);
            lv_obj_set_style_transform_pivot_y(emoji_label_, 0, 0);
            lv_label_set_long_mode(emoji_label_, LV_LABEL_LONG_CLIP);
            if (emoji_image_ != nullptr) {
                lv_obj_add_flag(emoji_image_, LV_OBJ_FLAG_HIDDEN);
            }
            if (status_label_ != nullptr) {
                lv_obj_remove_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }

    // Add theme switching function
    virtual void SetTheme(Theme* theme) override;
    
    // Set whether to hide chat messages/subtitles
    void SetHideSubtitle(bool hide);
};

// SPI LCD display
class SpiLcdDisplay : public LcdDisplay {
public:
    SpiLcdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                  int width, int height, int offset_x, int offset_y,
                  bool mirror_x, bool mirror_y, bool swap_xy);
};

// RGB LCD display
class RgbLcdDisplay : public LcdDisplay {
public:
    RgbLcdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                  int width, int height, int offset_x, int offset_y,
                  bool mirror_x, bool mirror_y, bool swap_xy);
};

// MIPI LCD display
class MipiLcdDisplay : public LcdDisplay {
public:
    MipiLcdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                   int width, int height, int offset_x, int offset_y,
                   bool mirror_x, bool mirror_y, bool swap_xy);
};

#endif // LCD_DISPLAY_H
