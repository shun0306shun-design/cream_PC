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
    EventFlags flags;       //!< Opened chests and other optional events (saved).
    uint8_t story_progress = progress::ch1_start;   //!< Main story position (saved), see rpg_story.h.
    StoryId story_battle = StoryId::none;   //!< Scene whose boss battle is being fought. Not saved.
    bool story_battle_lost = false;         //!< That battle was lost and the story goes on anyway. Not saved.
    MapId map = start_map;
    uint8_t player_x = start_x;
    uint8_t player_y = start_y;
    Direction facing = Direction::down;
    bn::random random;      //!< Not saved.

    void init_new_game();

    /// Moves the player to the respawn point of the current chapter (used after a defeat).
    void return_to_start();
};

/// Writes the game to SRAM.
void save_game(const GameState& state);

/**
 * Reads the game from SRAM. Returns false (leaving `state` untouched) if there's no valid save.
 * If the saved position is no longer walkable, the player is moved back to the start (see GameState::return_to_start).
 */
[[nodiscard]] bool load_game(GameState& state);

[[nodiscard]] bool has_saved_game();

}

#endif
