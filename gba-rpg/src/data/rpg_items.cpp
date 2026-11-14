#include "rpg_items.h"

#include "bn_assert.h"

#include "rpg_text.h"

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
        { RPG_T("Herb", "薬草"),         { EffectType::heal_hp, TargetType::one_ally,      25,    5 },       8 },
        { RPG_T("Magic Water", "魔法の水"),  { EffectType::heal_mp, TargetType::one_ally,      12,    4 },       30 },
        { RPG_T("Fire Bomb", "火炎玉"),    { EffectType::damage,  TargetType::all_enemies,   12,    6 },       20 },

        // Equipment: no effect; what matters is the kind, who can use it and the stat bonus.
        //  name              effect  price  kind              classes                                  HP MP ATK DEF AGI
        { RPG_T("Bronze Sword", "銅の剣"),     {},     50,    ItemKind::weapon, class_bit(ClassId::hero) | class_bit(ClassId::samurai),   { 0, 0, 5,  0,  0 } },
        { RPG_T("Katana", "刀"),           {},     120,   ItemKind::weapon, class_bit(ClassId::samurai),                              { 0, 0, 9,  0,  1 } },
        { RPG_T("Oak Staff", "樫の杖"),        {},     40,    ItemKind::weapon, class_bit(ClassId::priest) | class_bit(ClassId::mage),    { 0, 0, 3,  0,  0 } },
        { RPG_T("Leather Armor", "革の鎧"),    {},     40,    ItemKind::armor,  all_classes,                                              { 0, 0, 0,  4,  0 } },
        { RPG_T("Chain Mail", "鎖かたびら"),       {},     100,   ItemKind::armor,  class_bit(ClassId::hero) | class_bit(ClassId::samurai) | class_bit(ClassId::lancer), { 0, 0, 0, 8, 0 } },
        { RPG_T("Steel Sword", "鋼の剣"),      {},     150,   ItemKind::weapon, class_bit(ClassId::hero) | class_bit(ClassId::samurai),   { 0, 0, 8,  0,  0 } },
        { RPG_T("Iron Spear", "鉄の槍"),       {},     110,   ItemKind::weapon, class_bit(ClassId::lancer),                               { 0, 0, 6,  0,  2 } },
        { RPG_T("Star Sigil", "星の印章"),       {},     0,     ItemKind::key },
        { RPG_T("Rico's Charm", "リコのお守り"),   {},     0,     ItemKind::key },
        { RPG_T("Compass", "羅針盤"),            {},     0,     ItemKind::key },
    };

    static_assert(sizeof(items) / sizeof(items[0]) == item_count, "Item table doesn't match ItemId");
}

const ItemData& item_data(ItemId id)
{
    BN_ASSERT(id < ItemId::count, "Invalid item id: ", int(id));

    return items[int(id)];
}

}
