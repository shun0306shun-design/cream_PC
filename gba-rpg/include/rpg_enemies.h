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
    skeleton,
    cult_soldier,
    cult_scout,     //!< Boss of chapter 1.
    drake,          //!< Boss of chapter 2 (Ash General).
    star_guardian,  //!< Boss of the Star Ruins (chapter 3).
    warden,         //!< Boss of the waterway (chapter 3).
    cult_knight,
    ember_wyrm,     //!< Boss of the Searing Canyon (chapter 4).
    ash_zealot,
    zelma,          //!< Boss of the Ash Cathedral (chapter 5): she escapes.
    mine_overseer,  //!< Boss of the abandoned mine (chapter 5).
    general_2,      //!< General 2 in the capital (chapter 6): not meant to be beaten.
    drake_last,     //!< Boss of the pass (chapter 7): the first Ash General that dies.
    general_3,      //!< Bosses of the citadel (chapter 8).
    general_4,
    general_2_last,
    emperor,        //!< Final boss (chapter 9), first form.
    emperor_pyre,   //!< Final boss, second form.
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
