/*
 * Field menu (START button): status, items, magic and save.
 */

#ifndef RPG_FIELD_MENU_H
#define RPG_FIELD_MENU_H

namespace rpg
{

class Ui;
struct GameState;

/// Opens the field menu and returns when the player closes it.
void open_field_menu(GameState& state, Ui& ui);

}

#endif
