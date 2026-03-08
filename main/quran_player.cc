#include "quran_player.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

#include <driver/sdspi_host.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_vfs_fat.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "audio_codec.h"
#include "board.h"

extern "C" {
#include "esp_audio_dec_default.h"
#include "esp_audio_dec_reg.h"
#include "esp_audio_simple_dec.h"
#include "esp_audio_simple_dec_default.h"
}

namespace {

constexpr const char* TAG = "QuranPlayer";
constexpr const char* kMountPoint = "/sdcard";
constexpr const char* kQuranDirUpper = "/sdcard/Quran";
constexpr const char* kQuranDirLower = "/sdcard/quran";
constexpr size_t kReadBufferSize = 2048;
constexpr size_t kPcmBufferSize = 4096;
constexpr size_t kMaxPcmBufferSize = 16384;

const SurahInfo kSurahs[] = {
    {1, "al-fatiha", "fatiha", 7}, {2, "al-baqarah", "baqarah", 286}, {3, "ali-imran", "imran", 200},
    {4, "an-nisa", "nisa", 176}, {5, "al-maidah", "maidah", 120}, {6, "al-anam", "anam", 165},
    {7, "al-araf", "araf", 206}, {8, "al-anfal", "anfal", 75}, {9, "at-tawbah", "tawbah", 129},
    {10, "yunus", "yunus", 109}, {11, "hud", "hud", 123}, {12, "yusuf", "yusuf", 111},
    {13, "ar-rad", "rad", 43}, {14, "ibrahim", "ibrahim", 52}, {15, "al-hijr", "hijr", 99},
    {16, "an-nahl", "nahl", 128}, {17, "al-isra", "isra", 111}, {18, "al-kahf", "kahf", 110},
    {19, "maryam", "maryam", 98}, {20, "ta-ha", "taha", 135}, {21, "al-anbiya", "anbiya", 112},
    {22, "al-hajj", "hajj", 78}, {23, "al-muminun", "muminun", 118}, {24, "an-nur", "nur", 64},
    {25, "al-furqan", "furqan", 77}, {26, "ash-shuara", "shuara", 227}, {27, "an-naml", "naml", 93},
    {28, "al-qasas", "qasas", 88}, {29, "al-ankabut", "ankabut", 69}, {30, "ar-rum", "rum", 60},
    {31, "luqman", "luqman", 34}, {32, "as-sajdah", "sajdah", 30}, {33, "al-ahzab", "ahzab", 73},
    {34, "saba", "saba", 54}, {35, "fatir", "fatir", 45}, {36, "ya-sin", "yasin", 83},
    {37, "as-saffat", "saffat", 182}, {38, "sad", "sad", 88}, {39, "az-zumar", "zumar", 75},
    {40, "ghafir", "ghafir", 85}, {41, "fussilat", "fussilat", 54}, {42, "ash-shura", "shura", 53},
    {43, "az-zukhruf", "zukhruf", 89}, {44, "ad-dukhan", "dukhan", 59}, {45, "al-jathiyah", "jathiyah", 37},
    {46, "al-ahqaf", "ahqaf", 35}, {47, "muhammad", "muhammad", 38}, {48, "al-fath", "fath", 29},
    {49, "al-hujurat", "hujurat", 18}, {50, "qaf", "qaf", 45}, {51, "adh-dhariyat", "dhariyat", 60},
    {52, "at-tur", "tur", 49}, {53, "an-najm", "najm", 62}, {54, "al-qamar", "qamar", 55},
    {55, "ar-rahman", "rahman", 78}, {56, "al-waqiah", "waqiah", 96}, {57, "al-hadid", "hadid", 29},
    {58, "al-mujadilah", "mujadilah", 22}, {59, "al-hashr", "hashr", 24}, {60, "al-mumtahanah", "mumtahanah", 13},
    {61, "as-saff", "saff", 14}, {62, "al-jumuah", "jumuah", 11}, {63, "al-munafiqun", "munafiqun", 11},
    {64, "at-taghabun", "taghabun", 18}, {65, "at-talaq", "talaq", 12}, {66, "at-tahrim", "tahrim", 12},
    {67, "al-mulk", "mulk", 30}, {68, "al-qalam", "qalam", 52}, {69, "al-haqqah", "haqqah", 52},
    {70, "al-maarij", "maarij", 44}, {71, "nuh", "nuh", 28}, {72, "al-jinn", "jinn", 28},
    {73, "al-muzzammil", "muzzammil", 20}, {74, "al-muddaththir", "muddaththir", 56}, {75, "al-qiyamah", "qiyamah", 40},
    {76, "al-insan", "insan", 31}, {77, "al-mursalat", "mursalat", 50}, {78, "an-naba", "naba", 40},
    {79, "an-naziat", "naziat", 46}, {80, "abasa", "abasa", 42}, {81, "at-takwir", "takwir", 29},
    {82, "al-infitar", "infitar", 19}, {83, "al-mutaffifin", "mutaffifin", 36}, {84, "al-inshiqaq", "inshiqaq", 25},
    {85, "al-buruj", "buruj", 22}, {86, "at-tariq", "tariq", 17}, {87, "al-ala", "ala", 19},
    {88, "al-ghashiyah", "ghashiyah", 26}, {89, "al-fajr", "fajr", 30}, {90, "al-balad", "balad", 20},
    {91, "ash-shams", "shams", 15}, {92, "al-layl", "layl", 21}, {93, "ad-duha", "duha", 11},
    {94, "ash-sharh", "sharh", 8}, {95, "at-tin", "tin", 8}, {96, "al-alaq", "alaq", 19},
    {97, "al-qadr", "qadr", 5}, {98, "al-bayyinah", "bayyinah", 8}, {99, "az-zalzalah", "zalzalah", 8},
    {100, "al-adiyat", "adiyat", 11}, {101, "al-qariah", "qariah", 11}, {102, "at-takathur", "takathur", 8},
    {103, "al-asr", "asr", 3}, {104, "al-humazah", "humazah", 9}, {105, "al-fil", "fil", 5},
    {106, "quraysh", "quraysh", 4}, {107, "al-maun", "maun", 7}, {108, "al-kawthar", "kawthar", 3},
    {109, "al-kafirun", "kafirun", 6}, {110, "an-nasr", "nasr", 3}, {111, "al-masad", "masad", 5},
    {112, "al-ikhlas", "ikhlas", 4}, {113, "al-falaq", "falaq", 5}, {114, "an-nas", "nas", 6},
};

}  // namespace

QuranPlayer& QuranPlayer::GetInstance() {
    static QuranPlayer instance;
    return instance;
}

bool QuranPlayer::MountSdCard(spi_host_device_t spi_host,
                              int pin_sck,
                              int pin_mosi,
                              int pin_miso,
                              int pin_cs,
                              int max_transfer_size) {
    if (mounted_.load()) {
        return true;
    }

    ESP_LOGI(TAG, "SD mount start: host=%d sck=%d mosi=%d miso=%d cs=%d max_xfer=%d",
             static_cast<int>(spi_host), pin_sck, pin_mosi, pin_miso, pin_cs, max_transfer_size);

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = spi_host;
    host.max_freq_khz = 4000;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.host_id = spi_host;
    slot_config.gpio_cs = static_cast<gpio_num_t>(pin_cs);

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 6;
    mount_config.allocation_unit_size = 16 * 1024;
    mount_config.disk_status_check_enable = false;

    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num = pin_mosi;
    bus_cfg.miso_io_num = pin_miso;
    bus_cfg.sclk_io_num = pin_sck;
    bus_cfg.quadwp_io_num = GPIO_NUM_NC;
    bus_cfg.quadhd_io_num = GPIO_NUM_NC;
    bus_cfg.max_transfer_sz = max_transfer_size;

    esp_err_t err = spi_bus_initialize(spi_host, &bus_cfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(err));
        return false;
    }
    if (err == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "SPI bus already initialized (shared with display)");
    }

    err = esp_vfs_fat_sdspi_mount(kMountPoint, &host, &slot_config, &mount_config, &card_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed: %s", esp_err_to_name(err));
        return false;
    }

    esp_audio_dec_register_default();
    esp_audio_simple_dec_register_default();
    mounted_.store(true);
    ESP_LOGI(TAG, "SD mounted at %s", kMountPoint);
    return true;
}

bool QuranPlayer::Initialize(spi_host_device_t spi_host,
                             int pin_sck,
                             int pin_mosi,
                             int pin_miso,
                             int pin_cs,
                             int max_transfer_size) {
    return MountSdCard(spi_host, pin_sck, pin_mosi, pin_miso, pin_cs, max_transfer_size);
}

bool QuranPlayer::IsReady() const { return mounted_.load(); }

bool QuranPlayer::IsPlaying() const { return playing_.load(); }

std::string QuranPlayer::NormalizeName(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    return out;
}

const SurahInfo* QuranPlayer::FindSurahByIndex(uint8_t index) {
    for (const auto& surah : kSurahs) {
        if (surah.index == index) {
            return &surah;
        }
    }
    return nullptr;
}

bool QuranPlayer::ResolveSurah(const std::string& query, SurahInfo& info) const {
    std::string normalized = NormalizeName(query);
    if (normalized.empty()) {
        return false;
    }

    if (std::all_of(normalized.begin(), normalized.end(), [](char c) { return c >= '0' && c <= '9'; })) {
        int idx = std::atoi(normalized.c_str());
        if (idx >= 1 && idx <= 114) {
            const SurahInfo* by_index = FindSurahByIndex(static_cast<uint8_t>(idx));
            if (by_index != nullptr) {
                info = *by_index;
                return true;
            }
        }
    }

    for (const auto& surah : kSurahs) {
        if (NormalizeName(surah.name) == normalized || NormalizeName(surah.alias) == normalized) {
            info = surah;
            return true;
        }
    }
    return false;
}

cJSON* QuranPlayer::ResolveSurahJson(const std::string& query) const {
    cJSON* json = cJSON_CreateObject();
    SurahInfo info = {};
    bool ok = ResolveSurah(query, info);
    cJSON_AddBoolToObject(json, "found", ok);
    if (ok) {
        cJSON_AddNumberToObject(json, "index", info.index);
        cJSON_AddStringToObject(json, "name", info.name);
        cJSON_AddStringToObject(json, "alias", info.alias);
        cJSON_AddNumberToObject(json, "ayahCount", info.ayah_count);
    }
    return json;
}

cJSON* QuranPlayer::GetStatusJson() const {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddBoolToObject(json, "mounted", mounted_.load());
    cJSON_AddBoolToObject(json, "playing", playing_.load());
    cJSON_AddStringToObject(json, "mountPoint", kMountPoint);
    cJSON_AddStringToObject(json, "quranPathPrimary", kQuranDirUpper);
    cJSON_AddStringToObject(json, "quranPathFallback", kQuranDirLower);
    return json;
}

bool QuranPlayer::ResolveAyahQuery(uint8_t surah, const std::string& query, uint16_t& ayah) {
    std::string normalized = NormalizeName(query);
    if (normalized.empty()) {
        ayah = 1;
        return true;
    }

    if (std::all_of(normalized.begin(), normalized.end(), [](char c) { return c >= '0' && c <= '9'; })) {
        ayah = static_cast<uint16_t>(std::atoi(normalized.c_str()));
        return true;
    }

    if (surah == 2 && (normalized == "ayatalkursi" || normalized == "ayatulkursi" || normalized == "kursi")) {
        ayah = 255;
        return true;
    }

    return false;
}

bool QuranPlayer::BuildAyahPath(uint8_t surah, uint16_t ayah, std::string& out_path) {
    char rel[24];
    std::snprintf(rel, sizeof(rel), "/%03u/%03u.mp3", surah, ayah);

    std::string upper = std::string(kQuranDirUpper) + rel;
    FILE* fp = std::fopen(upper.c_str(), "rb");
    if (fp != nullptr) {
        std::fclose(fp);
        out_path = upper;
        return true;
    }

    std::string lower = std::string(kQuranDirLower) + rel;
    fp = std::fopen(lower.c_str(), "rb");
    if (fp != nullptr) {
        std::fclose(fp);
        out_path = lower;
        return true;
    }
    return false;
}

bool QuranPlayer::PlayByQuery(const std::string& surah_query,
                              const std::string& ayah_query,
                              std::string& error,
                              bool spawn_task) {
    SurahInfo info = {};
    if (!ResolveSurah(surah_query, info)) {
        error = "Unable to resolve surah from query";
        return false;
    }

    uint16_t ayah = 1;
    if (!ResolveAyahQuery(info.index, ayah_query, ayah)) {
        error = "Unable to resolve ayah from query";
        return false;
    }
    return PlayAyah(info.index, ayah, error, spawn_task);
}

bool QuranPlayer::PlayAyah(uint8_t surah, uint16_t ayah, std::string& error, bool spawn_task) {
    if (!mounted_.load()) {
        error = "SD card is not mounted";
        return false;
    }
    if (playing_.exchange(true)) {
        error = "Quran player is already playing";
        return false;
    }

    const SurahInfo* info = FindSurahByIndex(surah);
    if (info == nullptr) {
        error = "Surah must be between 1 and 114";
        playing_.store(false);
        return false;
    }
    if (ayah < 1 || ayah > info->ayah_count) {
        char buffer[96];
        std::snprintf(buffer, sizeof(buffer), "Ayah out of range for surah %u (max %u)", surah, info->ayah_count);
        error.assign(buffer);
        playing_.store(false);
        return false;
    }

    std::string path;
    if (!BuildAyahPath(surah, ayah, path)) {
        error = "Requested ayah file does not exist (/sdcard/Quran or /sdcard/quran)";
        playing_.store(false);
        return false;
    }

    if (spawn_task) {
        auto task_path = new std::string(path);
        if (xTaskCreate(&QuranPlayer::PlaybackTaskThunk, "quran_play", 8192, task_path, 3, nullptr) != pdPASS) {
            delete task_path;
            error = "Failed to create playback task";
            playing_.store(false);
            return false;
        }
    } else {
        try {
            PlaybackTask(path);
        } catch (...) {
            error = "Unexpected error while starting playback";
            playing_.store(false);
            return false;
        }
    }
    return true;
}

void QuranPlayer::PlaybackTaskThunk(void* arg) {
    std::unique_ptr<std::string> path(static_cast<std::string*>(arg));
    QuranPlayer::GetInstance().PlaybackTask(*path);
    vTaskDelete(nullptr);
}

void QuranPlayer::PlaybackTask(std::string path) {
    auto codec = Board::GetInstance().GetAudioCodec();
    if (codec == nullptr) {
        ESP_LOGE(TAG, "Audio codec unavailable");
        playing_.store(false);
        return;
    }

    FILE* fp = std::fopen(path.c_str(), "rb");
    if (fp == nullptr) {
        ESP_LOGE(TAG, "Cannot open file: %s", path.c_str());
        playing_.store(false);
        return;
    }

    esp_audio_simple_dec_cfg_t cfg = {};
    cfg.dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_MP3;
    cfg.use_frame_dec = false;

    esp_audio_simple_dec_handle_t dec = nullptr;
    esp_audio_err_t dec_err = esp_audio_simple_dec_open(&cfg, &dec);
    if (dec_err != ESP_AUDIO_ERR_OK || dec == nullptr) {
        ESP_LOGE(TAG, "esp_audio_simple_dec_open failed: %d", static_cast<int>(dec_err));
        std::fclose(fp);
        playing_.store(false);
        return;
    }

    std::vector<uint8_t> read_buf(kReadBufferSize);
    std::vector<uint8_t> pcm_buf(kPcmBufferSize);
    std::vector<int16_t> mono;
    std::vector<int16_t> resampled;

    bool eos = false;
    esp_audio_simple_dec_info_t info = {};
    bool info_ready = false;

    while (true) {
        size_t bytes_read = std::fread(read_buf.data(), 1, read_buf.size(), fp);
        eos = (bytes_read == 0);

        esp_audio_simple_dec_raw_t raw = {};
        raw.buffer = read_buf.data();
        raw.len = static_cast<uint32_t>(bytes_read);
        raw.eos = eos;

        do {
            uint32_t consumed_before = raw.consumed;
            esp_audio_simple_dec_out_t out = {};
            out.buffer = pcm_buf.data();
            out.len = static_cast<uint32_t>(pcm_buf.size());

            dec_err = esp_audio_simple_dec_process(dec, &raw, &out);
            if (dec_err == ESP_AUDIO_ERR_BUFF_NOT_ENOUGH) {
                if (out.needed_size > kMaxPcmBufferSize) {
                    ESP_LOGE(TAG, "PCM buffer too large: %u", out.needed_size);
                    eos = true;
                    break;
                }
                pcm_buf.resize(out.needed_size);
                continue;
            }
            if (dec_err != ESP_AUDIO_ERR_OK) {
                ESP_LOGE(TAG, "Decode error: %d", static_cast<int>(dec_err));
                eos = true;
                break;
            }

            if (!info_ready && out.decoded_size > 0) {
                if (esp_audio_simple_dec_get_info(dec, &info) == ESP_AUDIO_ERR_OK) {
                    info_ready = true;
                }
            }
            if (!info_ready || out.decoded_size == 0) {
                if (!eos && raw.consumed == consumed_before) {
                    break;
                }
                continue;
            }
            if (info.bits_per_sample != 16 || info.channel < 1) {
                ESP_LOGE(TAG, "Unsupported stream format bits=%u channels=%u",
                         info.bits_per_sample, info.channel);
                eos = true;
                break;
            }

            size_t samples_total = out.decoded_size / sizeof(int16_t);
            size_t frames = samples_total / info.channel;
            if (frames == 0) {
                continue;
            }

            const int16_t* in = reinterpret_cast<const int16_t*>(out.buffer);
            mono.resize(frames);
            for (size_t i = 0; i < frames; ++i) {
                int32_t acc = 0;
                for (uint8_t ch = 0; ch < info.channel; ++ch) {
                    acc += in[i * info.channel + ch];
                }
                mono[i] = static_cast<int16_t>(acc / info.channel);
            }

            int in_rate = static_cast<int>(info.sample_rate);
            int out_rate = codec->output_sample_rate();
            if (in_rate <= 0) {
                continue;
            }

            if (in_rate == out_rate) {
                codec->OutputData(mono);
            } else {
                size_t out_frames = (frames * out_rate) / in_rate;
                if (out_frames == 0) {
                    continue;
                }
                if (out_frames > (kMaxPcmBufferSize / sizeof(int16_t))) {
                    ESP_LOGE(TAG, "Resampled frame too large: %u", static_cast<unsigned>(out_frames));
                    eos = true;
                    break;
                }
                resampled.resize(out_frames);
                for (size_t i = 0; i < out_frames; ++i) {
                    float src_pos = static_cast<float>(i) * static_cast<float>(in_rate) / static_cast<float>(out_rate);
                    size_t p0 = static_cast<size_t>(src_pos);
                    size_t p1 = std::min(p0 + 1, frames - 1);
                    float frac = src_pos - static_cast<float>(p0);
                    float v = (1.0f - frac) * mono[p0] + frac * mono[p1];
                    resampled[i] = static_cast<int16_t>(v);
                }
                codec->OutputData(resampled);
            }
        } while (raw.consumed < raw.len || (eos && raw.len == 0));

        if (eos) {
            break;
        }
    }

    esp_audio_simple_dec_close(dec);
    std::fclose(fp);
    playing_.store(false);
    ESP_LOGI(TAG, "Playback done: %s", path.c_str());
}
