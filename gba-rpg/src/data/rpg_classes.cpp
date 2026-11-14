#include "rpg_classes.h"

#include "bn_assert.h"

#include "rpg_names.h"

namespace rpg
{

namespace
{
    constexpr SpellLearn hero_spells[] = {
        { 3, SpellId::heal },
        { 5, SpellId::fire },
    };

    constexpr SpellLearn priest_spells[] = {
        { 1, SpellId::heal },
        { 6, SpellId::heal_all },
    };

    constexpr SpellLearn mage_spells[] = {
        { 1, SpellId::fire },
        { 1, SpellId::heal },
        { 4, SpellId::blizzard },
        { 9, SpellId::heal_all },
    };

    // The order must match ClassId.
    constexpr ClassData classes[] = {
        //              level 1 stats            growth per level x10
        //  name        HP  MP  ATK DEF AGI      HP  MP  ATK DEF AGI     spells
        { RPG_NAME_HERO, { 30, 8,  14, 10, 8  },  { 60, 20, 25, 20, 15 },   hero_spells },
        { "Samurai",  { 36, 0,  18, 9,  10 },  { 70, 0,  32, 18, 20 },   {} },
        { RPG_NAME_DEFECTOR, { 24, 14, 9,  8,  7  },  { 45, 35, 14, 16, 14 },   priest_spells },
        { RPG_NAME_SEER, { 20, 18, 6,  6,  9  },  { 38, 42, 10, 12, 18 },   mage_spells },
        // The partner: strikes first and hard, but has less defense than the hero and no magic.
        { RPG_NAME_PARTNER, { 28, 0, 16, 8, 14 }, { 55, 0, 28, 15, 28 }, {} },
    };

    static_assert(sizeof(classes) / sizeof(classes[0]) == int(ClassId::count), "Class table doesn't match ClassId");

    [[nodiscard]] constexpr int16_t grow(int base, int growth_x10, int level)
    {
        return int16_t(base + ((growth_x10 * (level - 1)) / 10));
    }
}

const ClassData& class_data(ClassId id)
{
    BN_ASSERT(id < ClassId::count, "Invalid class id: ", int(id));

    return classes[int(id)];
}

Stats stats_for_level(ClassId id, int level)
{
    const ClassData& data = class_data(id);
    const Stats& base = data.base;
    const Stats& growth = data.growth_x10;

    return Stats{
        grow(base.max_hp, growth.max_hp, level),
        grow(base.max_mp, growth.max_mp, level),
        grow(base.attack, growth.attack, level),
        grow(base.defense, growth.defense, level),
        grow(base.agility, growth.agility, level),
    };
}

int exp_for_level(int level)
{
    // Simple quadratic curve: 10, 32, 66, 112, 170... Replace it with a table for finer control.
    int n = level - 1;
    return (6 * n * n) + (4 * n);
}

}
