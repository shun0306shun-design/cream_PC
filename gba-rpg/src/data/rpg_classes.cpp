#include "rpg_classes.h"

#include "bn_assert.h"

#include "rpg_text.h"
#include "rpg_names.h"

namespace rpg
{

namespace
{
    // Skills learned by level (placeholders: more will be added later).
    // A character can know up to max_known_spells (rpg_party.h) skills.
    constexpr SpellLearn hero_spells[] = {
        { 5,  SpellId::charge },
        { 10, SpellId::sweep },
        { 15, SpellId::mineuchi },
        { 20, SpellId::full_slash },
        { 30, SpellId::ultimate_slash },
    };

    constexpr SpellLearn sion_spells[] = {
        { 6,  SpellId::pierce },
        { 12, SpellId::double_thrust },
        { 18, SpellId::whirlwind_spear },
        { 28, SpellId::infinite_spear },
    };

    // The seer: recovery magic.
    constexpr SpellLearn mirea_spells[] = {
        { 4,  SpellId::heal },
        { 9,  SpellId::behoimi },
        { 14, SpellId::behoma },
        { 19, SpellId::zaoral },
        { 27, SpellId::full_restore },
    };

    // The former cult priest: fire magic.
    constexpr SpellLearn nox_spells[] = {
        { 5,  SpellId::mera },
        { 10, SpellId::merami },
        { 15, SpellId::merazoma },
        { 30, SpellId::doom_flame },
    };

    // The order must match ClassId.
    constexpr ClassData classes[] = {
        //              level 1 stats            growth per level x10
        //  name        HP  MP  ATK DEF AGI      HP  MP  ATK DEF AGI     skills
        { RPG_T(RPG_NAME_HERO, RPG_NAME_HERO_JA), { 30, 8,  14, 10, 8  },  { 60, 20, 25, 20, 15 },   hero_spells },
        { RPG_T("Samurai", "サムライ"),  { 36, 0,  18, 9,  10 },  { 70, 0,  32, 18, 20 },   {} },
        { RPG_T(RPG_NAME_DEFECTOR, RPG_NAME_DEFECTOR_JA), { 24, 14, 9,  8,  7  },  { 45, 35, 14, 16, 14 },   nox_spells },
        { RPG_T(RPG_NAME_SEER, RPG_NAME_SEER_JA), { 20, 18, 6,  6,  9  },  { 38, 42, 10, 12, 18 },   mirea_spells },
        // The partner: strikes first and hard, but has less defense than the hero. MP only for spear techniques.
        { RPG_T(RPG_NAME_PARTNER, RPG_NAME_PARTNER_JA), { 28, 6, 16, 8, 14 }, { 55, 15, 28, 15, 28 }, sion_spells },
    };

    static_assert(sizeof(classes) / sizeof(classes[0]) == int(ClassId::count), "Class table doesn't match ClassId");

    static_assert(exp_for_level(1) == 0 && exp_for_level(max_level) == 3901392, "Unexpected experience curve");

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

}
