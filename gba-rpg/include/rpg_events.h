/*
 * Map event handlers: what happens when the player talks to, opens or uses something on the map.
 *
 * FieldScene finds the event in front of the player and calls the matching function here, so the scene
 * only deals with movement, camera and encounters. To add an event type: add it to EventType (rpg_maps.h),
 * write its handler here and call it from FieldScene::_interact().
 *
 * Every handler blocks until it is done and may open windows on the lower part of the screen.
 */

#ifndef RPG_EVENTS_H
#define RPG_EVENTS_H

#include "rpg_maps.h"
#include "rpg_ui.h"

namespace bn
{
    class sprite_ptr;
}

namespace rpg
{

struct GameState;

// Where ask_yes_no() puts its menu, in cells: just above the message box, at the right.
// FieldScene checks (static_assert) that the minimap doesn't reach it.
constexpr int yes_no_cols = 8;
constexpr int yes_no_rows = Window::rows_for_lines(2);
constexpr int yes_no_col = screen_columns - yes_no_cols;
constexpr int yes_no_row = screen_rows - MessageBox::box_rows - yes_no_rows;

/**
 * Shows a Yes/No menu and returns true for "Yes" (B counts as "No").
 * Call it while a MessageBox with the question is on screen.
 */
[[nodiscard]] bool ask_yes_no(Ui& ui);

/// Like ask_yes_no(), with two custom options (a bit wider). Returns true for the first one (B chooses the second).
[[nodiscard]] bool ask_choice(Ui& ui, const bn::string_view& first, const bn::string_view& second);

/// Like ask_choice(), but it tells the options apart from B: returns 0, 1 or -1 (cancelled).
[[nodiscard]] int ask_option(Ui& ui, const bn::string_view& first, const bn::string_view& second);

/// EventType::talk and EventType::sign: shows the pages (the ones of the current chapter, if it has them).
void run_talk(const GameState& state, Ui& ui, const MapEvent& event);

/// EventType::inn: asks, charges and fully restores the party (it also revives fallen members).
void run_inn(GameState& state, Ui& ui, const MapEvent& event);

/// EventType::spring: restores HP/MP of the living members for `price` gold (it can't revive).
void use_spring(GameState& state, Ui& ui, const MapEvent& event);

/**
 * EventType::chest: gives the contents once and remembers it with the event flag.
 * @param chest_sprite The chest sprite, switched to its "open" frame when the chest opens (can be null).
 * @return true if the chest was opened now, false if it was already open or couldn't be opened.
 */
bool open_chest(GameState& state, Ui& ui, const MapEvent& event, bn::sprite_ptr* chest_sprite);

}

#endif
