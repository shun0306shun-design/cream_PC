/*
 * Enemy database and encounter groups.
 * To add an enemy: add an id here, a sprite in graphics/ and a row in src/data/rpg_enemies.cpp.
 */

#ifndef RPG_ENEMIES_H
#define RPG_ENEMIES_H

#include "rpg_spells.h"

namespace bn
{
    class sprite_item;
}

namespace rpg
{

enum class EnemyId : uint8_t
{
    slime,
    bat,
    goblin,
    count
};

struct EnemyData
{
    bn::string_view name;
    const bn::sprite_item* sprite;      //!< 32x32 battle sprite.
    Stats stats;
    int16_t exp;
    int16_t gold;
    SpellId spell;                      //!< Spell the enemy may cast...
    uint8_t spell_chance;               //!< ...with this chance in percent (0 = never).
};

/// A set of enemies that appear together.
struct EncounterGroup
{
    EnemyId enemies[max_enemies];
    uint8_t count;
};

[[nodiscard]] const EnemyData& enemy_data(EnemyId id);

}

#endif
