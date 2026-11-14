/*
 * Character class database (hero, samurai, priest, mage, lancer).
 * Story characters are classes too: the hero class is the hero, the lancer is the partner (names in rpg_names.h).
 *
 * Stats are never stored per character: they are always computed from class + level,
 * so rebalancing only needs a table edit and save files stay tiny.
 */

#ifndef RPG_CLASSES_H
#define RPG_CLASSES_H

#include "bn_span.h"

#include "rpg_spells.h"

namespace rpg
{

enum class ClassId : uint8_t
{
    hero,
    samurai,
    priest,     //!< The former cult priest (chapter 5): fire magic.
    mage,       //!< The seer (chapter 3): recovery magic.
    lancer,     //!< The partner: fast spear fighter with spear techniques.
    count
};

/// Bit of a class in a "which classes" mask (see ItemData::classes).
[[nodiscard]] constexpr uint8_t class_bit(ClassId id)
{
    return uint8_t(1 << int(id));
}

constexpr uint8_t all_classes = uint8_t((1 << int(ClassId::count)) - 1);

struct SpellLearn
{
    uint8_t level;      //!< Level at which the spell is learned.
    SpellId spell;
};

struct ClassData
{
    bn::string_view name;
    Stats base;                         //!< Stats at level 1.
    Stats growth_x10;                   //!< Gain per level, multiplied by 10 (25 = +2.5 per level).
    bn::span<const SpellLearn> spells;
};

[[nodiscard]] const ClassData& class_data(ClassId id);

[[nodiscard]] Stats stats_for_level(ClassId id, int level);

/**
 * Total experience required to reach the given level (level 1 = 0).
 *
 * Dragon Quest like cubic curve: the first levels come quickly and every level asks for more than the last one.
 *   Lv 2: 12   Lv 5: 312   Lv 10: 3,132   Lv 20: 28,272   Lv 40: 240,552   Lv 60: 828,832   Lv 100: 3,901,392
 */
[[nodiscard]] constexpr int exp_for_level(int level)
{
    int n = level - 1;
    return (4 * n * n * n) + (2 * n * n) + (6 * n);
}

/**
 * Experience an ordinary enemy should give if it lives in an area meant for parties of the given level.
 * Use it in the enemy table instead of hand written numbers to keep the pace of the whole game:
 *
 * A level takes about 2 + level * 0.3 battles against 2-3 enemies of the party's own level
 * (Lv 1: 2 battles, Lv 10: 5, Lv 30: 11, Lv 60: 20), which adds up to ~670 battles from Lv 1 to Lv 60.
 * With the usual boss bonuses, a normal playthrough finishes the main story around Lv 55-65
 * if the last area is designed for Lv 55-60. Levels 61-100 are for the post-game.
 *
 * @param percent Scales the result: ~1000 for a boss, 50 for a weak enemy that comes in big groups...
 */
[[nodiscard]] constexpr int enemy_exp_for_level(int level, int percent = 100)
{
    int next_level_exp = exp_for_level(level + 1) - exp_for_level(level);
    int battles_x10 = 20 + (level * 3);
    int exp = int((int64_t(next_level_exp) * percent) / (battles_x10 * 22));     // 2.2 enemies per battle.
    return exp < 1 ? 1 : exp;
}

}

#endif
