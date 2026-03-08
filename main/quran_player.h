#ifndef QURAN_PLAYER_H_
#define QURAN_PLAYER_H_

#include <atomic>
#include <cstdint>
#include <string>

#include <cJSON.h>
#include <driver/spi_common.h>
#include <sdmmc_cmd.h>

struct SurahInfo {
    uint8_t index;
    const char* name;
    const char* alias;
    uint16_t ayah_count;
};

class QuranPlayer {
public:
    static QuranPlayer& GetInstance();

    bool Initialize(spi_host_device_t spi_host,
                    int pin_sck,
                    int pin_mosi,
                    int pin_miso,
                    int pin_cs,
                    int max_transfer_size);

    bool IsReady() const;
    bool IsPlaying() const;

    bool ResolveSurah(const std::string& query, SurahInfo& info) const;
    cJSON* ResolveSurahJson(const std::string& query) const;
    cJSON* GetStatusJson() const;

    bool PlayAyah(uint8_t surah, uint16_t ayah, std::string& error, bool spawn_task = true);
    bool PlayByQuery(const std::string& surah_query,
                     const std::string& ayah_query,
                     std::string& error,
                     bool spawn_task = true);

private:
    QuranPlayer() = default;
    QuranPlayer(const QuranPlayer&) = delete;
    QuranPlayer& operator=(const QuranPlayer&) = delete;

    bool MountSdCard(spi_host_device_t spi_host,
                     int pin_sck,
                     int pin_mosi,
                     int pin_miso,
                     int pin_cs,
                     int max_transfer_size);

    static std::string NormalizeName(const std::string& input);
    static const SurahInfo* FindSurahByIndex(uint8_t index);
    static bool ResolveAyahQuery(uint8_t surah, const std::string& query, uint16_t& ayah);
    static bool BuildAyahPath(uint8_t surah, uint16_t ayah, std::string& out_path);
    static void PlaybackTaskThunk(void* arg);
    void PlaybackTask(std::string path);

    std::atomic<bool> mounted_{false};
    std::atomic<bool> playing_{false};
    sdmmc_card_t* card_ = nullptr;
};

#endif  // QURAN_PLAYER_H_
