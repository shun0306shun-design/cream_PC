#include "rpg_items.h"

#include "bn_assert.h"

namespace rpg
{

namespace
{
    // The order must match ItemId.
    //
    // Balance guide: a level 1 party member has 20-36 HP and 0-18 MP, so a Herb (25 HP) is almost a full heal early
    // on and shops sell it for 8 G. Healing is the same Effect structure used by spells: `power` + [0, `variance`].
    // Prices are the BUY price (selling pays sell_percent of it, see rpg_shop.h).
    constexpr ItemData items[] = {
        //  name            effect type           target                    power  variance   price
        { "Herb",         { EffectType::heal_hp, TargetType::one_ally,      25,    5 },       8 },
        { "Magic Water",  { EffectType::heal_mp, TargetType::one_ally,      12,    4 },       30 },
        { "Fire Bomb",    { EffectType::damage,  TargetType::all_enemies,   12,    6 },       20 },
    };

    static_assert(sizeof(items) / sizeof(items[0]) == item_count, "Item table doesn't match ItemId");
}

const ItemData& item_data(ItemId id)
{
    BN_ASSERT(id < ItemId::count, "Invalid item id: ", int(id));

    return items[int(id)];
}

}
