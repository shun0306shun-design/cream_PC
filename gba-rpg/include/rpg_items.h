/*
 * Item database. To add an item: add an id here and a row in src/data/rpg_items.cpp.
 *
 * Equipment (weapons and armor) are items too: they are bought, sold, found in chests and counted in the
 * bag like any other item. Equipping one moves it from the bag to a slot of a party member (rpg_party.h).
 */

#ifndef RPG_ITEMS_H
#define RPG_ITEMS_H

#include "bn_string_view.h"

#include "rpg_classes.h"
#include "rpg_common.h"

namespace rpg
{

enum class ItemId : uint8_t
{
    herb,
    magic_water,
    fire_bomb,
    bronze_sword,
    katana,
    oak_staff,
    leather_armor,
    chain_mail,
    steel_sword,
    iron_spear,
    star_sigil,     //!< Key item: opens the waterway under the gate (chapter 3).
    rico_charm,     //!< E701: keepsake from the child who saved the hero (meant to become an accessory).
    count,
    none = 0xFF         //!< "Nothing": an empty equipment slot.
};

constexpr int item_count = int(ItemId::count);

/// Capacity of the bag, in kinds of items. It is part of the save data, so it doesn't change when items are added.
constexpr int max_items = 32;

static_assert(item_count <= max_items, "More items than the bag can hold: raise max_items (and save_version)");

enum class ItemKind : uint8_t
{
    consumable,     //!< Used from the item menus: `effect` applies.
    weapon,         //!< Equipment slot 1.
    armor,          //!< Equipment slot 2.
    key             //!< Story item: can't be used, equipped or sold (price 0).
};

struct ItemData
{
    bn::string_view name;
    Effect effect;
    int16_t price;      //!< Buy price in shops. The sell price is a percentage of it (rpg_shop.h).

    // --- Equipment only (kind != consumable) ---
    ItemKind kind = ItemKind::consumable;
    uint8_t classes = 0;        //!< Classes that can equip it, as a mask of class_bit() values.
    Stats bonus = {};           //!< Added to the wearer's stats (attack, defense, agility, max HP/MP).

    [[nodiscard]] constexpr bool is_equipment() const
    {
        return kind == ItemKind::weapon || kind == ItemKind::armor;
    }
};

[[nodiscard]] const ItemData& item_data(ItemId id);

}

#endif
