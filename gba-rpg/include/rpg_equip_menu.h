/*
 * Equipment menu (field menu > Equip): choose a party member, then a slot, then what to wear.
 *
 * Equipment data lives in the item table (rpg_items.cpp) and the slots in PartyMember (rpg_party.h).
 */

#ifndef RPG_EQUIP_MENU_H
#define RPG_EQUIP_MENU_H

namespace rpg
{

class Ui;
struct GameState;

/// Opens the equipment menu next to the field menu and returns when the player closes it.
void open_equip_menu(GameState& state, Ui& ui);

}

#endif
