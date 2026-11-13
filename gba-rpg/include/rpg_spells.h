/*
 * Spell database. To add a spell: add an id here and a row in src/data/rpg_spells.cpp.
 */

#ifndef RPG_SPELLS_H
#define RPG_SPELLS_H

#include "bn_string_view.h"

#include "rpg_common.h"

namespace rpg
{

enum class SpellId : uint8_t
{
    heal,
    heal_all,
    fire,
    blizzard,
    count
};

struct SpellData
{
    bn::string_view name;
    uint8_t mp_cost;
    Effect effect;
};

[[nodiscard]] const SpellData& spell_data(SpellId id);

}

#endif
