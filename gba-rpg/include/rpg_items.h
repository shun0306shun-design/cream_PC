/*
 * Item database. To add an item: add an id here and a row in src/data/rpg_items.cpp.
 */

#ifndef RPG_ITEMS_H
#define RPG_ITEMS_H

#include "bn_string_view.h"

#include "rpg_common.h"

namespace rpg
{

enum class ItemId : uint8_t
{
    herb,
    magic_water,
    fire_bomb,
    count
};

constexpr int item_count = int(ItemId::count);

struct ItemData
{
    bn::string_view name;
    Effect effect;
    int16_t price;      //!< Buy price in shops. The sell price is a percentage of it (rpg_shop.h).
};

[[nodiscard]] const ItemData& item_data(ItemId id);

}

#endif
