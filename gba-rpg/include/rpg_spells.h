/*
 * Skill database ("spell" in code): everything used with the Skill command, paid with MP.
 * Weapon techniques and magic share it, the difference is only the effect type.
 *
 * To add a skill: add an id here, a row in src/data/rpg_spells.cpp
 * and the level at which it's learned in src/data/rpg_classes.cpp.
 * The ids are never shown: to rename a skill only its row in rpg_spells.cpp has to change.
 */

#ifndef RPG_SPELLS_H
#define RPG_SPELLS_H

#include "bn_string_view.h"

#include "rpg_common.h"

namespace rpg
{

enum class SpellId : uint8_t
{
    // Hero
    charge,
    sweep,
    mineuchi,
    full_slash,
    ultimate_slash,
    // The partner
    pierce,
    double_thrust,
    whirlwind_spear,
    infinite_spear,
    // The seer
    heal,
    behoimi,
    behoma,
    zaoral,
    full_restore,
    // The former cult priest
    mera,
    merami,
    merazoma,
    doom_flame,
    // Enemies only
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
