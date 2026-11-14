/*
 * Event flags: persistent yes/no facts about the game world ("this chest was opened", an optional event...).
 * The main story does NOT use flags: it is one number, GameState::story_progress (see rpg_story.h).
 *
 * To add a flag: add a name to FlagId and use it from map data (MapEvent::flag) or from code.
 * Flags live in GameState and are saved with the game. Never reorder or remove existing ids once
 * players may have saves: new flags go before `count`, and unused ones can simply stay.
 */

#ifndef RPG_FLAGS_H
#define RPG_FLAGS_H

#include "bn_assert.h"

#include "rpg_common.h"

namespace rpg
{

enum class FlagId : uint8_t
{
    chest_village_herbs,
    chest_field_gold,
    chest_village_sword,
    chest_field_katana,
    chest_cave_1f,
    chest_cave_2f,
    chest_cave_3f,
    chest_fort,
    chest_ruins,
    chest_waterway,
    chest_canyon,
    chest_cathedral,
    chest_pass,
    chest_citadel,
    // Add new flags here.
    count,
    none = 0xFF         //!< "No flag": the default of MapEvent::flag.
};

constexpr int max_flags = 64;   //!< Capacity of EventFlags (8 bytes of save data). Raise it if you need more.

static_assert(int(FlagId::count) <= max_flags, "Too many flags: raise max_flags");

/// A fixed size bit set. Plain data (trivially copyable) so it can be saved to SRAM as is.
struct EventFlags
{
    uint8_t bits[max_flags / 8] = {};

    [[nodiscard]] bool get(FlagId id) const
    {
        BN_ASSERT(id < FlagId::count, "Invalid flag id: ", int(id));

        return bits[int(id) / 8] & (1 << (int(id) % 8));
    }

    void set(FlagId id)
    {
        BN_ASSERT(id < FlagId::count, "Invalid flag id: ", int(id));

        bits[int(id) / 8] = uint8_t(bits[int(id) / 8] | (1 << (int(id) % 8)));
    }
};

}

#endif
