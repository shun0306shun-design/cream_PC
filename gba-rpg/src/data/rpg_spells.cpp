#include "rpg_spells.h"

#include "bn_assert.h"

namespace rpg
{

namespace
{
    // The order must match SpellId.
    constexpr SpellData spells[] = {
        //  name        MP  effect type           target                    power  variance
        { "Heal",       3, { EffectType::heal_hp, TargetType::one_ally,     20,    8 } },
        { "Heal All",   8, { EffectType::heal_hp, TargetType::all_allies,   16,    6 } },
        { "Fire",       2, { EffectType::damage,  TargetType::one_enemy,    10,    6 } },
        { "Blizzard",   5, { EffectType::damage,  TargetType::all_enemies,  9,     5 } },
    };

    static_assert(sizeof(spells) / sizeof(spells[0]) == int(SpellId::count), "Spell table doesn't match SpellId");
}

const SpellData& spell_data(SpellId id)
{
    BN_ASSERT(id < SpellId::count, "Invalid spell id: ", int(id));

    return spells[int(id)];
}

}
