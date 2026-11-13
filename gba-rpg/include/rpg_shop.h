/*
 * Item shop: buy and sell menus.
 *
 * Prices live in the item table (ItemData::price, src/data/rpg_items.cpp): that is the BUY price.
 * The SELL price is a percentage of it (sell_percent below). What each shop sells is its `stock`
 * list in the map data (MapEvent::stock), so every town can have its own.
 */

#ifndef RPG_SHOP_H
#define RPG_SHOP_H

#include "bn_span.h"
#include "bn_string_view.h"

#include "rpg_items.h"

namespace rpg
{

class Ui;
struct GameState;

/// Shops pay this percentage of the buy price. Change it to make selling more or less generous.
constexpr int sell_percent = 50;

/// What the shop pays for one unit of an item (0 = not accepted).
[[nodiscard]] int sell_price(ItemId id);

/**
 * Runs the shop until the player leaves (blocks).
 * @param stock Items on sale. Must outlive the call.
 * @param greeting Message pages shown when entering (a default one if empty).
 *
 * It uses the whole top of the screen: close any corner window before calling it.
 */
void open_shop(GameState& state, Ui& ui, const bn::span<const ItemId>& stock,
               const bn::span<const bn::string_view>& greeting);

}

#endif
