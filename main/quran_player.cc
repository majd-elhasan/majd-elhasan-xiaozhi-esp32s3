#include "quran_player.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/spi_common.h>

#include "audio_codec.h"
#include "board.h"

namespace {

constexpr const char* TAG = "QuranPlayer";
// Audio content now expected to be provided by a remote MCP client device
// (e.g., a companion with its own storage). Local filesystem paths are not used.
constexpr const char* kMountPoint = "remote";
constexpr const char* kQuranDirUpper = "remote";
constexpr const char* kQuranDirLower = "remote";

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
    (void)spi_host;
    (void)pin_sck;
    (void)pin_mosi;
    (void)pin_miso;
    (void)pin_cs;
    (void)max_transfer_size;

    // SD/flash on this device is unused; audio lives on a remote MCP client.
    mounted_.store(true);
    ESP_LOGI(TAG, "Quran playback uses remote MCP client storage; local mount skipped");
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
    ESP_LOGD(TAG, "ResolveSurah: query='%s' normalized='%s'", query.c_str(), normalized.c_str());
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
            ESP_LOGI(TAG, "ResolveSurah: matched name='%s' alias='%s' index=%u", surah.name, surah.alias, surah.index);
            return true;
        }
    }
    ESP_LOGW(TAG, "ResolveSurah: no match for '%s'", query.c_str());
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
    cJSON_AddStringToObject(json, "source", "remote_mcp_client");
    cJSON_AddStringToObject(json, "mountPoint", kMountPoint);
    cJSON_AddStringToObject(json, "quranPathPrimary", kQuranDirUpper);
    cJSON_AddStringToObject(json, "quranPathFallback", kQuranDirLower);
    return json;
}

bool QuranPlayer::ResolveAyahQuery(uint8_t surah, const std::string& query, uint16_t& ayah) {
    std::string normalized = NormalizeName(query);
    ESP_LOGD(TAG, "ResolveAyahQuery: surah=%u query='%s' normalized='%s'", surah, query.c_str(), normalized.c_str());
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
        ESP_LOGI(TAG, "ResolveAyahQuery: special ayah detected (ayat al-kursi) => ayah=255");
        return true;
    }
    if (surah == 2 && (normalized == "ayataddain" || normalized == "ayatuddain" || normalized == "dayn" || normalized == "dain")) {
        ayah = 282;
        ESP_LOGI(TAG, "ResolveAyahQuery: special ayah detected (ayat ad-dain) => ayah=282");
        return true;
    }

    ESP_LOGW(TAG, "ResolveAyahQuery: could not resolve ayah from '%s' for surah=%u", query.c_str(), surah);
    return false;
}

bool QuranPlayer::BuildAyahPath(uint8_t surah, uint16_t ayah, std::string& out_path) {
    char rel[24];
    std::snprintf(rel, sizeof(rel), "/%03u/%03u", surah, ayah);
    // Path is just an identifier to pass via MCP to the remote client.
    out_path = std::string("remote") + rel;
    return true;
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
    ESP_LOGI(TAG, "PlayAyah requested: surah=%u ayah=%u spawn=%d", surah, ayah, spawn_task ? 1 : 0);
    if (!mounted_.load()) {
        mounted_.store(true);
    }
    if (playing_.exchange(true)) {
        error = "Quran player is already playing";
        ESP_LOGW(TAG, "PlayAyah failed: already playing");
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
    BuildAyahPath(surah, ayah, path);
    ESP_LOGI(TAG, "Delegating Quran playback to cloud source: %s", path.c_str());
    // Local audio is disabled; assume cloud MCP service performs playback.
    playing_.store(false);
    return true;
}

void QuranPlayer::PlaybackTaskThunk(void* arg) {
    std::unique_ptr<std::string> path(static_cast<std::string*>(arg));
    QuranPlayer::GetInstance().PlaybackTask(*path);
    vTaskDelete(nullptr);
}

void QuranPlayer::PlaybackTask(std::string path) {
    (void)path;
    // Local playback is disabled; cloud service should stream audio.
    playing_.store(false);
    ESP_LOGI(TAG, "PlaybackTask skipped (cloud-managed)");
}
