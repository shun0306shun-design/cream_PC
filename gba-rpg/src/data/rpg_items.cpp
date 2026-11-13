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

        // Equipment: no effect; what matters is the kind, who can use it and the stat bonus.
        //  name              effect  price  kind              classes                                  HP MP ATK DEF AGI
        { "Bronze Sword",     {},     50,    ItemKind::weapon, class_bit(ClassId::hero) | class_bit(ClassId::samurai),   { 0, 0, 5,  0,  0 } },
        { "Katana",           {},     120,   ItemKind::weapon, class_bit(ClassId::samurai),                              { 0, 0, 9,  0,  1 } },
        { "Oak Staff",        {},     40,    ItemKind::weapon, class_bit(ClassId::priest) | class_bit(ClassId::mage),    { 0, 0, 3,  0,  0 } },
        { "Leather Armor",    {},     40,    ItemKind::armor,  all_classes,                                              { 0, 0, 0,  4,  0 } },
        { "Chain Mail",       {},     100,   ItemKind::armor,  class_bit(ClassId::hero) | class_bit(ClassId::samurai),   { 0, 0, 0,  8,  0 } },
        { "Steel Sword",      {},     150,   ItemKind::weapon, class_bit(ClassId::hero) | class_bit(ClassId::samurai),   { 0, 0, 8,  0,  0 } },
    };

    static_assert(sizeof(items) / sizeof(items[0]) == item_count, "Item table doesn't match ItemId");
}

const ItemData& item_data(ItemId id)
{
    BN_ASSERT(id < ItemId::count, "Invalid item id: ", int(id));

    return items[int(id)];
}

}
