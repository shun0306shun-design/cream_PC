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
    goblin_king,    //!< Sample boss.
    dark_lord,      //!< Sample last boss.
    count
};

/// How dangerous an enemy is. It decides how many times it can act per round (see enemy_action_chances).
enum class EnemyRank : uint8_t
{
    normal,
    boss,
    last_boss,
    count
};

constexpr int max_enemy_actions = 3;

/// Chances in percent of acting 1, 2 or 3 times in a round. They must add up to 100.
struct EnemyActionChances
{
    uint8_t percent[max_enemy_actions];
};

[[nodiscard]] const EnemyActionChances& enemy_action_chances(EnemyRank rank);

struct EnemyData
{
    bn::string_view name;
    const bn::sprite_item* sprite;      //!< 32x32 battle sprite.
    Stats stats;
    EnemyRank rank;
    int32_t exp;
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
