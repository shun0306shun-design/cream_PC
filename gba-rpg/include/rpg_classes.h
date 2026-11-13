/*
 * Character class database (hero, samurai, priest, mage).
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
    priest,
    mage,
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

/// Total experience required to reach the given level (level 1 = 0).
[[nodiscard]] int exp_for_level(int level);

}

#endif
