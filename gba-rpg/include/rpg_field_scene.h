/*
 * Map exploration: tile map rendering, grid movement, collisions, events and random encounters.
 */

#ifndef RPG_FIELD_SCENE_H
#define RPG_FIELD_SCENE_H

#include "bn_vector.h"
#include "bn_optional.h"
#include "bn_camera_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_item.h"

#include "rpg_maps.h"
#include "rpg_ui.h"

namespace rpg
{

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

    // Convenience UI. Both are optional so they can be closed while other windows need that space.
    bn::optional<Window> _name_window;      //!< Map name, shown for a moment after entering a map.
    int _name_frames = 0;                   //!< Frames left before the map name disappears.
    bn::optional<Window> _minimap;          //!< Always visible, top-right corner.
    bool _minimap_dirty = true;
    int _minimap_x = -1;                    //!< Player position the minimap was last drawn for.
    int _minimap_y = -1;

    void _load_map();
    void _update_view();

    void _show_map_name();
    void _update_map_name();
    void _update_minimap();

    /// Closes the corner windows and reopens them later: needed before opening bigger windows.
    void _hide_hud();
    void _show_hud();

    [[nodiscard]] bool _can_walk(int tile_x, int tile_y) const;
    [[nodiscard]] const MapEvent* _event_at(int tile_x, int tile_y) const;

    void _handle_movement_input();
    void _interact();

    /// Index in _npcs of the sprite of an event with graphics.
    [[nodiscard]] int _npc_sprite_index(const MapEvent& event) const;
    void _warp(const MapEvent& event);

    /// Called when the player arrives to a new tile. Returns the encounter to fight, if any.
    [[nodiscard]] const EncounterGroup* _on_step();
};

}

#endif
