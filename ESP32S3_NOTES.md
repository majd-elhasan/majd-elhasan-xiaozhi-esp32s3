# ESP32-S3 Notes

## Font and Subtitle Notes

- Current status:
  - Arabic text renders correctly on the 240x240 SPI LCD.
  - English and CJK text (Chinese/Korean path) are shown with the default built-in text font.
  - Chat/subtitle font is selected dynamically by script:
    - Arabic script -> Arabic font (`lv_font_dejavu_16_persian_hebrew`)
    - Other scripts -> default built-in text font (`BUILTIN_TEXT_FONT`)

- Known item to improve:
  - In language-learning scenarios, mixed-language subtitles should always show both languages correctly.
  - Example requirement:
    - If UI/current language is Arabic and learning language is Korean, both Arabic and Korean lines should be visible on screen at the same time.
  - We may still need stronger mixed-font fallback logic if any Korean text is missing in Arabic-selected flows.

## Next Planned Stage

- After this pushed version, next step is SD-card model work:
  - Activate SD model flow.
  - Make the AI model play Quran audio tracks from the SD card.
