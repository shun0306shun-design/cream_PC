/*
 * Global game state shared by all the scenes, plus save/load to cartridge SRAM.
 */

#ifndef RPG_GAME_STATE_H
#define RPG_GAME_STATE_H

#include "bn_random.h"

#include "rpg_flags.h"
#include "rpg_maps.h"
#include "rpg_party.h"

namespace rpg
{

struct GameState
{
    Party party;
    EventFlags flags;       //!< Opened chests and other world events (saved).
    MapId map = start_map;
    uint8_t player_x = start_x;
    uint8_t player_y = start_y;
    Direction facing = Direction::down;
    bn::random random;      //!< Not saved.

    void init_new_game();

    /// Moves the player to the start position (used after a defeat).
    void return_to_start();
};

/// Writes the game to SRAM.
void save_game(const GameState& state);

/// Reads the game from SRAM. Returns false (leaving `state` untouched) if there's no valid save.
[[nodiscard]] bool load_game(GameState& state);

[[nodiscard]] bool has_saved_game();

}

#endif
