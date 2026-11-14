/*
 * Music and sound effects.
 *
 * The game never names an audio file: it asks for a BgmId or a SeId, and the tables of src/rpg_audio.cpp say
 * which file of audio/ that is. To replace a placeholder, overwrite the file keeping its name
 * (bgm_*.mod / .xm / .it / .s3m, se_*.wav) and rebuild. To add a new one: add an id here, a file in audio/
 * and a row in the table.
 *
 * Volumes and mute are settings of the player: they are stored in SRAM as soon as they change, apart from
 * the saved game (so they survive a new game, and changing them never touches the save).
 *
 * No heap is used: the state is a few bytes of static memory.
 */

#ifndef RPG_AUDIO_H
#define RPG_AUDIO_H

#include "bn_common.h"

namespace rpg
{

enum class SeId : uint8_t
{
    confirm,    //!< A menu option was chosen.
    cancel,     //!< A menu was closed with B.
    attack,     //!< Someone attacks with a weapon (party or enemies).
    damage,     //!< Someone takes damage.
    level_up,
    chest,      //!< A chest opens.
    count
};

enum class BgmId : uint8_t
{
    none,           //!< Silence.
    town,           //!< Towns and villages.
    dungeon,        //!< Every map with random battles.
    battle,         //!< Random battles.
    boss,           //!< Battles with a boss or last boss rank enemy.
    event_dark,     //!< Important scenes: loss, ruin.
    event_hope,     //!< Important scenes: resolve, the ending.
    count
};

namespace audio
{
    constexpr int max_volume_level = 4;     //!< Volume levels go from 0 (silent) to this.

    /// Where the settings are stored in SRAM. The saved game (rpg_game_state.cpp) must end before this.
    constexpr int sram_settings_offset = 1024;

    /// Loads the settings from SRAM. Call it once, after bn::core::init().
    void init();

    /// Starts a looping music, unless it is the one that is already playing. BgmId::none stops the music.
    void play_bgm(BgmId id);

    /// The music chosen with play_bgm() (even if it can't be heard because of the settings).
    [[nodiscard]] BgmId current_bgm();

    void play_se(SeId id);

    [[nodiscard]] int bgm_volume_level();
    [[nodiscard]] int se_volume_level();
    [[nodiscard]] bool muted();

    // Setters apply the change at once and store the settings in SRAM.
    void set_bgm_volume_level(int level);
    void set_se_volume_level(int level);
    void set_muted(bool muted);
}

}

#endif
