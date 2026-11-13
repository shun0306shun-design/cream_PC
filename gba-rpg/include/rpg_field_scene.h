/*
 * Map exploration: tile map rendering, grid movement, collisions, events and random encounters.
 */

#ifndef RPG_FIELD_SCENE_H
#define RPG_FIELD_SCENE_H

#include "bn_vector.h"
#include "bn_camera_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_item.h"

#include "rpg_maps.h"

namespace rpg
{

class Ui;
struct GameState;

class FieldScene
{

public:
    FieldScene(GameState& state, Ui& ui);

    /// Runs the exploration loop until a random battle starts. Returns the enemies to fight.
    [[nodiscard]] const EncounterGroup& run();

private:
    GameState& _state;
    Ui& _ui;
    const MapData* _map;
    bn::camera_ptr _camera;
    bn::regular_bg_map_item _map_item;
    bn::regular_bg_ptr _bg;
    bn::regular_bg_map_ptr _bg_map;
    bn::sprite_ptr _player;
    bn::vector<bn::sprite_ptr, max_map_npcs> _npcs;
    int _pixel_x = 0;           //!< Player position in map pixels (top-left corner of its tile).
    int _pixel_y = 0;
    int _move_frames = 0;       //!< Frames left of the current step (0 = standing).
    int _walk_counter = 0;
    int _safe_steps = 0;        //!< Steps without encounters left.

    void _load_map();
    void _update_view();

    [[nodiscard]] bool _can_walk(int tile_x, int tile_y) const;
    [[nodiscard]] const MapEvent* _event_at(int tile_x, int tile_y) const;

    void _handle_movement_input();
    void _interact();
    void _run_inn(const MapEvent& event);
    void _warp(const MapEvent& event);

    /// Called when the player arrives to a new tile. Returns the encounter to fight, if any.
    [[nodiscard]] const EncounterGroup* _on_step();
};

}

#endif
