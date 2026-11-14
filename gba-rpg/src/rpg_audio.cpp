#include "rpg_audio.h"

#include "bn_assert.h"
#include "bn_fixed.h"
#include "bn_music.h"
#include "bn_sram.h"
#include "bn_music_items.h"
#include "bn_sound_items.h"

namespace rpg::audio
{

namespace
{
    // -----------------------------------------------------------------------------------------------------------------
    // Which file of audio/ is each id. `volume` balances the files against each other (0 to 1):
    // the player's volume level is applied on top of it.
    // -----------------------------------------------------------------------------------------------------------------
    struct BgmData
    {
        bn::music_item item;
        bn::fixed volume;
    };

    struct SeData
    {
        bn::sound_item item;
        bn::fixed volume;
    };

    // The order must match BgmId (without BgmId::none).
    constexpr BgmData bgm_table[] = {
        { bn::music_items::bgm_town,        0.6 },
        { bn::music_items::bgm_dungeon,     0.6 },
        { bn::music_items::bgm_battle,      0.6 },
        { bn::music_items::bgm_boss,        0.6 },
        { bn::music_items::bgm_event_dark,  0.6 },
        { bn::music_items::bgm_event_hope,  0.6 },
    };

    static_assert(sizeof(bgm_table) / sizeof(bgm_table[0]) == int(BgmId::count) - 1, "Music table doesn't match BgmId");

    // The order must match SeId.
    constexpr SeData se_table[] = {
        { bn::sound_items::se_confirm,      0.7 },
        { bn::sound_items::se_cancel,       0.7 },
        { bn::sound_items::se_attack,       1 },
        { bn::sound_items::se_damage,       1 },
        { bn::sound_items::se_level_up,     0.8 },
        { bn::sound_items::se_chest,        0.8 },
    };

    static_assert(sizeof(se_table) / sizeof(se_table[0]) == int(SeId::count), "Sound table doesn't match SeId");

    // -----------------------------------------------------------------------------------------------------------------
    // Settings
    // -----------------------------------------------------------------------------------------------------------------
    constexpr uint32_t settings_magic = 0x31445541;     // "AUD1"
    constexpr int default_volume_level = 3;

    /// What is stored in SRAM. Must stay trivially copyable.
    struct Settings
    {
        uint32_t magic = settings_magic;
        uint8_t bgm_level = default_volume_level;
        uint8_t se_level = default_volume_level;
        uint8_t muted = 0;
        uint8_t reserved = 0;
    };

    Settings settings;
    BgmId current = BgmId::none;

    void store_settings()
    {
        bn::sram::write_offset(settings, sram_settings_offset);
    }

    [[nodiscard]] bn::fixed level_factor(int level)
    {
        return settings.muted ? bn::fixed(0) : bn::fixed(level) / max_volume_level;
    }

    /// Makes what is heard match `current` and the settings.
    void apply_bgm()
    {
        bn::fixed volume = 0;

        if(current != BgmId::none)
        {
            volume = bgm_table[int(current) - 1].volume * level_factor(settings.bgm_level);
        }

        if(volume <= 0)
        {
            if(bn::music::playing())
            {
                bn::music::stop();
            }
        }
        else if(bn::music::playing())
        {
            bn::music::set_volume(volume);
        }
        else
        {
            bgm_table[int(current) - 1].item.play(volume, true);
        }
    }
}

void init()
{
    Settings stored;
    bn::sram::read_offset(stored, sram_settings_offset);

    // An empty or damaged SRAM keeps the defaults.
    if(stored.magic == settings_magic && stored.bgm_level <= max_volume_level &&
            stored.se_level <= max_volume_level && stored.muted <= 1)
    {
        settings = stored;
    }
}

void play_bgm(BgmId id)
{
    BN_ASSERT(id < BgmId::count, "Invalid music id: ", int(id));

    if(id == current)
    {
        return;
    }

    if(bn::music::playing())
    {
        bn::music::stop();
    }

    current = id;
    apply_bgm();
}

BgmId current_bgm()
{
    return current;
}

void play_se(SeId id)
{
    BN_ASSERT(id < SeId::count, "Invalid sound id: ", int(id));

    const SeData& data = se_table[int(id)];
    bn::fixed volume = data.volume * level_factor(settings.se_level);

    if(volume > 0)
    {
        // Optional: if every sound channel is busy the effect is skipped, which is better than an error.
        (void) data.item.play_optional(volume);
    }
}

int bgm_volume_level()
{
    return settings.bgm_level;
}

int se_volume_level()
{
    return settings.se_level;
}

bool muted()
{
    return settings.muted;
}

void set_bgm_volume_level(int level)
{
    BN_ASSERT(level >= 0 && level <= max_volume_level, "Invalid volume level: ", level);

    settings.bgm_level = uint8_t(level);
    apply_bgm();
    store_settings();
}

void set_se_volume_level(int level)
{
    BN_ASSERT(level >= 0 && level <= max_volume_level, "Invalid volume level: ", level);

    settings.se_level = uint8_t(level);
    store_settings();
}

void set_muted(bool muted)
{
    settings.muted = muted;
    apply_bgm();
    store_settings();
}

}
