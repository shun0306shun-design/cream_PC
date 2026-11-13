#include "rpg_field_scene.h"

#include "bn_keypad.h"
#include "bn_string.h"
#include "bn_sstream.h"
#include "bn_bg_tiles.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_cell_info.h"

#include "bn_sprite_items_npc.h"
#include "bn_sprite_items_player.h"
#include "bn_bg_palette_items_field_palette.h"
#include "bn_regular_bg_tiles_items_field_tiles.h"

#include "rpg_ui.h"
#include "rpg_field_menu.h"
#include "rpg_game_state.h"

namespace rpg
{

namespace
{
    constexpr int walk_speed = 2;                           // Pixels per frame.
    constexpr int step_frames = tile_size / walk_speed;
    constexpr int safe_steps_after_battle = 4;
    constexpr int inn_price = 10;

    // The map background is 64x64 cells (512x512 pixels): enough for max_map_tiles x max_map_tiles tiles.
    // 8 KB that live in EWRAM, never in the stack.
    constexpr int bg_cells = max_map_tiles * 2;
    constexpr int bg_half_size = bg_cells * 4;              // In pixels.

    alignas(int) BN_DATA_EWRAM_BSS bn::regular_bg_map_cell field_cells[bg_cells * bg_cells];

    constexpr int direction_dx[] = { 0, 0, -1, 1 };         // Indexed by Direction.
    constexpr int direction_dy[] = { 1, -1, 0, 0 };

    /// Expands the 16x16 tiles of a map into 8x8 background cells.
    void fill_cells(const MapData& map, const bn::regular_bg_map_item& map_item)
    {
        for(int tile_y = 0; tile_y < max_map_tiles; ++tile_y)
        {
            for(int tile_x = 0; tile_x < max_map_tiles; ++tile_x)
            {
                // Each map tile is made of 4 consecutive hardware tiles (see tools/gen_placeholder_assets.py).
                int first_tile_index = int(map.tile(tile_x, tile_y)) * 4;

                for(int quarter = 0; quarter < 4; ++quarter)
                {
                    int cell_x = (tile_x * 2) + (quarter % 2);
                    int cell_y = (tile_y * 2) + (quarter / 2);
                    bn::regular_bg_map_cell_info cell_info;
                    cell_info.set_tile_index(first_tile_index + quarter);
                    field_cells[map_item.cell_index(cell_x, cell_y)] = cell_info.cell();
                }
            }
        }
    }

    [[nodiscard]] bn::regular_bg_ptr create_bg(const MapData& map, const bn::regular_bg_map_item& map_item)
    {
        fill_cells(map, map_item);

        // Dynamic maps reference tiles by raw index, so the tiles must not be offset in VRAM.
        bn::bg_tiles::set_allow_offset(false);

        bn::regular_bg_item bg_item(
                    bn::regular_bg_tiles_items::field_tiles, bn::bg_palette_items::field_palette, map_item);
        bn::regular_bg_ptr bg = bg_item.create_bg(0, 0);
        bg.set_priority(scene_bg_priority);

        bn::bg_tiles::set_allow_offset(true);
        return bg;
    }

    /// Map pixels -> world coordinates (the background is centered in the world origin).
    [[nodiscard]] constexpr int world(int map_pixel)
    {
        return map_pixel - bg_half_size;
    }

    [[nodiscard]] constexpr int clamp_camera(int target, int map_size, int screen_size)
    {
        if(map_size <= screen_size)
        {
            return map_size / 2;
        }

        int half_screen = screen_size / 2;

        if(target < half_screen)
        {
            return half_screen;
        }

        return target > map_size - half_screen ? map_size - half_screen : target;
    }
}

FieldScene::FieldScene(GameState& state, Ui& ui) :
    _state(state),
    _ui(ui),
    _map(&map_data(state.map)),
    _camera(bn::camera_ptr::create(0, 0)),
    _map_item(field_cells[0], bn::size(bg_cells, bg_cells)),
    _bg(create_bg(*_map, _map_item)),
    _bg_map(_bg.map()),
    _player(bn::sprite_items::player.create_sprite(0, 0)),
    _safe_steps(safe_steps_after_battle)
{
    // Everything in the world follows the camera; the UI doesn't.
    _bg.set_camera(_camera);
    _player.set_camera(_camera);
    _player.set_bg_priority(scene_sprite_bg_priority);
    _load_map();
}

const EncounterGroup& FieldScene::run()
{
    _ui.fade_in();

    while(true)
    {
        // Advancing the generator every frame makes encounters depend on the player's timing.
        _state.random.update();

        if(_move_frames > 0)
        {
            // Finish the current step before reading input again: movement is grid based.
            _pixel_x += direction_dx[int(_state.facing)] * walk_speed;
            _pixel_y += direction_dy[int(_state.facing)] * walk_speed;
            ++_walk_counter;
            --_move_frames;

            if(_move_frames == 0)
            {
                if(const EncounterGroup* encounter = _on_step())
                {
                    _update_view();
                    _ui.fade_out();
                    return *encounter;
                }
            }
        }
        else if(bn::keypad::a_pressed())
        {
            _interact();
        }
        else if(bn::keypad::start_pressed())
        {
            open_field_menu(_state, _ui);
        }
        else
        {
            _handle_movement_input();
        }

        _update_view();
        _ui.update();
    }
}

void FieldScene::_load_map()
{
    _map = &map_data(_state.map);

    fill_cells(*_map, _map_item);
    _bg_map.reload_cells_ref();

    // One sprite per event with graphics.
    _npcs.clear();

    for(const MapEvent& event : _map->events)
    {
        if(event.npc_graphics >= 0)
        {
            BN_ASSERT(! _npcs.full(), "Too many NPCs in map: ", _map->name);

            bn::sprite_ptr npc = bn::sprite_items::npc.create_sprite(
                        world(event.x * tile_size) + (tile_size / 2),
                        world(event.y * tile_size) + (tile_size / 2), event.npc_graphics);
            npc.set_camera(_camera);
            npc.set_bg_priority(scene_sprite_bg_priority);
            _npcs.push_back(bn::move(npc));
        }
    }

    _pixel_x = _state.player_x * tile_size;
    _pixel_y = _state.player_y * tile_size;
    _move_frames = 0;
    _update_view();
}

void FieldScene::_update_view()
{
    // Player sprite: 2 animation frames per direction.
    int walk_frame = _move_frames > 0 ? (_walk_counter / step_frames) % 2 : 0;
    _player.set_tiles(bn::sprite_items::player.tiles_item(), (int(_state.facing) * 2) + walk_frame);
    _player.set_position(world(_pixel_x) + (tile_size / 2), world(_pixel_y) + (tile_size / 2));

    // The camera follows the player, but it doesn't show what is outside the map.
    int camera_x = clamp_camera(_pixel_x + (tile_size / 2), _map->width() * tile_size, screen_width);
    int camera_y = clamp_camera(_pixel_y + (tile_size / 2), _map->height() * tile_size, screen_height);
    _camera.set_position(world(camera_x), world(camera_y));
}

bool FieldScene::_can_walk(int tile_x, int tile_y) const
{
    if(tile_x < 0 || tile_y < 0 || tile_x >= _map->width() || tile_y >= _map->height())
    {
        return false;
    }

    if(! tile_info(_map->tile(tile_x, tile_y)).walkable)
    {
        return false;
    }

    // NPCs are solid.
    const MapEvent* event = _event_at(tile_x, tile_y);
    return ! event || event->npc_graphics < 0;
}

const MapEvent* FieldScene::_event_at(int tile_x, int tile_y) const
{
    for(const MapEvent& event : _map->events)
    {
        if(event.x == tile_x && event.y == tile_y)
        {
            return &event;
        }
    }

    return nullptr;
}

void FieldScene::_handle_movement_input()
{
    Direction direction;

    if(bn::keypad::down_held())
    {
        direction = Direction::down;
    }
    else if(bn::keypad::up_held())
    {
        direction = Direction::up;
    }
    else if(bn::keypad::left_held())
    {
        direction = Direction::left;
    }
    else if(bn::keypad::right_held())
    {
        direction = Direction::right;
    }
    else
    {
        return;
    }

    // Always turn, even if the way is blocked: that's how you face NPCs and signs.
    _state.facing = direction;

    int target_x = _state.player_x + direction_dx[int(direction)];
    int target_y = _state.player_y + direction_dy[int(direction)];

    if(_can_walk(target_x, target_y))
    {
        // The logical position changes at once, the sprite catches up during the next frames.
        _state.player_x = uint8_t(target_x);
        _state.player_y = uint8_t(target_y);
        _move_frames = step_frames;
    }
}

void FieldScene::_interact()
{
    int target_x = _state.player_x + direction_dx[int(_state.facing)];
    int target_y = _state.player_y + direction_dy[int(_state.facing)];
    const MapEvent* event = _event_at(target_x, target_y);

    if(! event)
    {
        return;
    }

    // To add a new kind of event (shop, chest, cutscene...): add an EventType and handle it here.
    switch(event->type)
    {

    case EventType::talk:
    case EventType::sign:
        {
            MessageBox box(_ui);
            box.say_pages(event->pages);
        }
        break;

    case EventType::inn:
        _run_inn(*event);
        break;

    case EventType::warp:
    default:
        break;      // Warps are triggered by stepping on them, see _on_step().
    }
}

void FieldScene::_run_inn(const MapEvent& event)
{
    static constexpr bn::string_view yes_no[] = { "Yes", "No" };

    MessageBox box(_ui);
    box.say_pages(event.pages);

    bn::string<64> question;
    bn::ostringstream question_stream(question);
    question_stream << "A night costs " << inn_price << " gold.\nDo you want to rest?";
    box.show(question);

    bool accepted;

    {
        int menu_rows = Window::rows_for_lines(2);
        Menu menu(_ui, screen_columns - 8, screen_rows - MessageBox::box_rows - menu_rows, 8, yes_no);
        accepted = menu.run() == 0;
    }

    if(! accepted)
    {
        box.say("Come back any time.");
    }
    else if(_state.party.gold < inn_price)
    {
        box.say("You don't have enough gold.");
    }
    else
    {
        _state.party.gold -= inn_price;
        _ui.fade_out();
        _state.party.restore_all();
        _ui.wait(30);
        _ui.fade_in();
        box.say("The party is fully rested!");
    }
}

void FieldScene::_warp(const MapEvent& event)
{
    _ui.fade_out();
    _state.map = event.warp_map;
    _state.player_x = event.warp_x;
    _state.player_y = event.warp_y;
    _load_map();
    _safe_steps = safe_steps_after_battle;
    _ui.update();
    _ui.fade_in();
}

const EncounterGroup* FieldScene::_on_step()
{
    const MapEvent* event = _event_at(_state.player_x, _state.player_y);

    if(event && event->type == EventType::warp)
    {
        _warp(*event);
        return nullptr;
    }

    // Random encounter roll.
    int tile_rate = tile_info(_map->tile(_state.player_x, _state.player_y)).encounter;

    if(_map->encounters.empty() || tile_rate == 0)
    {
        return nullptr;
    }

    if(_safe_steps > 0)
    {
        --_safe_steps;
        return nullptr;
    }

    if(_state.random.get_int(256) >= _map->encounter_rate * tile_rate)
    {
        return nullptr;
    }

    return &_map->encounters[_state.random.get_int(_map->encounters.size())];
}

}
