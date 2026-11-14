#include "rpg_field_scene.h"

#include "bn_keypad.h"
#include "bn_string.h"
#include "bn_bg_tiles.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_cell_info.h"

#include "bn_sprite_items_npc.h"
#include "bn_sprite_items_player.h"
#include "bn_bg_palette_items_field_palette.h"
#include "bn_regular_bg_tiles_items_field_tiles.h"

#include "rpg_ui.h"
#include "rpg_events.h"
#include "rpg_field_menu.h"
#include "rpg_game_state.h"
#include "rpg_shop.h"

namespace rpg
{

namespace
{
    constexpr int walk_speed = 2;                           // Pixels per frame.
    constexpr int step_frames = tile_size / walk_speed;
    constexpr int safe_steps_after_battle = 4;

    // --- Walking NPCs: slower than the player, and they rest between steps ---
    constexpr int npc_walk_speed = 1;                       // Pixels per frame.
    constexpr int npc_step_frames = tile_size / npc_walk_speed;
    constexpr int npc_min_wait = 30;                        // Frames standing still between steps...
    constexpr int npc_extra_wait = 90;                      // ...plus up to this many more (random).

    // --- Map name popup (top center) ---
    constexpr int name_frames = 105;        // About 1.75 seconds.
    constexpr int name_min_cols = 6;
    constexpr int name_max_cols = 14;       // Longer names would run into the minimap.

    // --- Minimap (top right): the tiles around the player as text, one character per tile ---
    constexpr int minimap_radius_x = 2;
    constexpr int minimap_radius_y = 1;
    constexpr int minimap_chars = (minimap_radius_x * 2) + 1;
    constexpr int minimap_lines = (minimap_radius_y * 2) + 1;
    constexpr int minimap_cols = ((minimap_chars * 8) + 16 + 7) / 8;    // Text + margins, in cells.
    constexpr int minimap_col = screen_columns - minimap_cols;
    constexpr int minimap_rows = Window::rows_for_lines(minimap_lines);

    // Windows must not overlap (see rpg_ui.h): the widest map name has to end before the minimap starts.
    static_assert(((screen_columns - name_max_cols) / 2) + name_max_cols <= minimap_col, "Name window overlaps minimap");

    // The Yes/No menu of the events (inn, spring) opens while the minimap stays on screen: the bottom of
    // the minimap must be at or above the first row of that menu.
    static_assert(minimap_rows <= yes_no_row, "Minimap overlaps the Yes/No menu");

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
    _map(&map_data(state.map, state.story_progress)),
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
    _show_hud();
}

const EncounterGroup& FieldScene::run()
{
    _ui.fade_in();

    // Story scenes that don't wait for the player: what follows a boss battle that was just won,
    // and the opening of a new game.
    StoryId start_story = StoryId::none;

    if(_state.story_battle != StoryId::none)
    {
        StoryId won = _state.story_battle;
        uint8_t old_progress = _state.story_progress;
        _state.story_battle = StoryId::none;
        run_story_victory(won, _state, _ui);

        if(_state.story_progress != old_progress)
        {
            _spawn_npcs();      // The boss is gone, allies may have appeared...
        }
    }
    else if(_state.story_progress == progress::ch1_start)
    {
        start_story = StoryId::ch1_opening;
    }

    if(const EncounterGroup* encounter = _run_story(start_story))
    {
        _ui.fade_out();
        return *encounter;
    }

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
            if(const EncounterGroup* encounter = _interact())
            {
                _update_view();
                _ui.fade_out();
                return *encounter;
            }
        }
        else if(bn::keypad::start_pressed())
        {
            // The menu uses the top of the screen: the corner windows can't stay under it.
            _hide_hud();
            open_field_menu(_state, _ui);
            _show_hud();
        }
        else
        {
            _handle_movement_input();
        }

        _update_walkers();
        _update_view();
        _update_map_name();
        _update_minimap();
        _ui.update();
    }
}

void FieldScene::_load_map()
{
    _map = &map_data(_state.map, _state.story_progress);

    fill_cells(*_map, _map_item);
    _bg_map.reload_cells_ref();

    _pixel_x = _state.player_x * tile_size;
    _pixel_y = _state.player_y * tile_size;
    _move_frames = 0;
    _update_view();
    _spawn_npcs();

    // Called at scene start and after every warp, since both go through here.
    _show_map_name();
}

void FieldScene::_spawn_npcs()
{
    int story_progress = _state.story_progress;

    // One sprite per event with graphics (only the events that exist at this point of the story).
    _walkers.clear();
    _npcs.clear();

    for(const MapEvent& event : _map->events)
    {
        if(event.npc_graphics >= 0 && event.active(story_progress))
        {
            BN_ASSERT(! _npcs.full(), "Too many NPCs in map: ", _map->name);

            // A chest that was already opened starts with its "open" frame.
            int graphics = event.npc_graphics;

            if(event.type == EventType::chest && _state.flags.get(event.flag))
            {
                ++graphics;
            }

            bn::sprite_ptr npc = bn::sprite_items::npc.create_sprite(
                        world(event.x * tile_size) + (tile_size / 2),
                        world(event.y * tile_size) + (tile_size / 2), graphics);
            npc.set_camera(_camera);
            npc.set_bg_priority(scene_sprite_bg_priority);
            _npcs.push_back(bn::move(npc));
        }
    }

    // Walking NPCs start at the place given in the map data (their position isn't saved).
    int sprite_index = 0;

    for(const MapEvent& event : _map->events)
    {
        if(event.npc_graphics >= 0 && event.active(story_progress))
        {
            if(event.movement != NpcMovement::stand)
            {
                BN_ASSERT(! _walkers.full(), "Too many walking NPCs in map: ", _map->name);

                int spawn_x = event.x;
                int spawn_y = event.y;
                _find_spawn_tile(event, spawn_x, spawn_y);

                if(spawn_x != event.x || spawn_y != event.y)
                {
                    // The sprite was created at the start tile: move it to where the NPC really is.
                    _npcs[sprite_index].set_position(world(spawn_x * tile_size) + (tile_size / 2),
                                                     world(spawn_y * tile_size) + (tile_size / 2));
                }

                _walkers.push_back(Walker{ &event, sprite_index, spawn_x, spawn_y, spawn_x, spawn_y,
                                           spawn_x * tile_size, spawn_y * tile_size, 0, 0, 0,
                                           _state.random.get_int(npc_extra_wait) + 1 });
            }

            ++sprite_index;
        }
    }

    _minimap_dirty = true;
}

void FieldScene::_show_map_name()
{
    const bn::string_view name = localized(_map->name);

    // Fit the window to the text (names use the proportional font), centered at the top of the screen.
    int cols = (_ui.text_generator().width(name) + 16 + 7) / 8;
    cols = cols < name_min_cols ? name_min_cols : (cols > name_max_cols ? name_max_cols : cols);

    _name_window.reset();   // Erase the previous name first, if it's still there.
    _name_window.emplace(_ui, (screen_columns - cols) / 2, 0, cols, Window::rows_for_lines(1));
    _name_window->print_centered(0, name);
    _name_frames = name_frames;
}

void FieldScene::_update_map_name()
{
    // The player can keep walking while the name is shown: it's just a timer.
    if(_name_frames > 0 && --_name_frames == 0)
    {
        _name_window.reset();
    }
}

void FieldScene::_update_minimap()
{
    if(! _minimap)
    {
        return;
    }

    // Redraw only when the player enters another tile (or the map changed): text sprites aren't free.
    int player_x = _state.player_x;
    int player_y = _state.player_y;

    if(! _minimap_dirty && player_x == _minimap_x && player_y == _minimap_y)
    {
        return;
    }

    _minimap_dirty = false;
    _minimap_x = player_x;
    _minimap_y = player_y;
    _minimap->clear_text();

    for(int line = 0; line < minimap_lines; ++line)
    {
        bn::string<minimap_chars> text;

        for(int column = 0; column < minimap_chars; ++column)
        {
            int dx = column - minimap_radius_x;
            int dy = line - minimap_radius_y;

            if(dx == 0 && dy == 0)
            {
                text.push_back('@');
            }
            else
            {
                // Same rule as the movement: out of the map, walls and NPCs block the way.
                text.push_back(_can_walk(player_x + dx, player_y + dy) ? '.' : '#');
            }
        }

        _minimap->print_fixed(line, text);
    }
}

void FieldScene::_hide_hud()
{
    _name_window.reset();
    _name_frames = 0;
    _minimap.reset();
}

void FieldScene::_show_hud()
{
    if(! _minimap)
    {
        _minimap.emplace(_ui, minimap_col, 0, minimap_cols, minimap_rows);
        _minimap_dirty = true;
    }
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
    if(! _map->terrain_walkable(tile_x, tile_y) || _walker_blocks(tile_x, tile_y))
    {
        return false;
    }

    // NPCs and chests that stay in place are solid. The ones that walk are handled by _walker_blocks().
    for(const MapEvent& event : _map->events)
    {
        if(event.npc_graphics >= 0 && event.movement == NpcMovement::stand && event.x == tile_x &&
           event.y == tile_y && event.active(_state.story_progress))
        {
            return false;
        }
    }

    return true;
}

const MapEvent* FieldScene::_event_at(int tile_x, int tile_y) const
{
    for(const MapEvent& event : _map->events)
    {
        if(! event.active(_state.story_progress))
        {
            continue;       // It doesn't exist at this point of the story.
        }

        // A walking NPC is where it is now, not where the map data says it started.
        int event_x = event.x;
        int event_y = event.y;

        if(const Walker* walker = _walker_for(event))
        {
            event_x = walker->x;
            event_y = walker->y;
        }

        if(event_x == tile_x && event_y == tile_y)
        {
            return &event;
        }
    }

    return nullptr;
}

const FieldScene::Walker* FieldScene::_walker_for(const MapEvent& event) const
{
    for(const Walker& walker : _walkers)
    {
        if(walker.event == &event)
        {
            return &walker;
        }
    }

    return nullptr;
}

bool FieldScene::_walker_blocks(int tile_x, int tile_y) const
{
    for(const Walker& walker : _walkers)
    {
        // The tile it is going to and, while it's still moving, the tile it is leaving.
        if((walker.x == tile_x && walker.y == tile_y) ||
           (walker.frames_left > 0 && walker.prev_x == tile_x && walker.prev_y == tile_y))
        {
            return true;
        }
    }

    return false;
}

void FieldScene::_find_spawn_tile(const MapEvent& event, int& spawn_x, int& spawn_y) const
{
    if(event.x != _state.player_x || event.y != _state.player_y)
    {
        return;     // The usual case: nobody is on its start tile.
    }

    // Look at the rings of tiles around the start tile, closest first, within its walking range.
    for(int distance = 1; distance <= event.wander_radius; ++distance)
    {
        for(int dy = -distance; dy <= distance; ++dy)
        {
            for(int dx = -distance; dx <= distance; ++dx)
            {
                int tile_x = event.x + dx;
                int tile_y = event.y + dy;

                if(bn::max(bn::abs(dx), bn::abs(dy)) == distance &&
                   _map->terrain_walkable(tile_x, tile_y) &&
                   ! _event_at(tile_x, tile_y) &&               // Chests, signs, warps and other NPCs
                   ! _walker_blocks(tile_x, tile_y) &&
                   ! (tile_x == _state.player_x && tile_y == _state.player_y))
                {
                    spawn_x = tile_x;
                    spawn_y = tile_y;
                    return;
                }
            }
        }
    }

    // No room at all (not in this game's maps): it starts on the player and walks off later.
}

void FieldScene::_update_walkers()
{
    // Cheap by design: most frames a walker only counts down; the decision is made when it stops waiting.
    for(Walker& walker : _walkers)
    {
        if(walker.frames_left > 0)
        {
            walker.px += walker.dx * npc_walk_speed;
            walker.py += walker.dy * npc_walk_speed;
            _npcs[walker.sprite_index].set_position(world(walker.px) + (tile_size / 2),
                                                    world(walker.py) + (tile_size / 2));

            if(--walker.frames_left == 0)
            {
                walker.wait = npc_min_wait + _state.random.get_int(npc_extra_wait);

                // It left the tile behind: if that is on the minimap, show it as free.
                _minimap_dirty = _minimap_dirty ||
                        (bn::abs(walker.prev_x - _state.player_x) <= minimap_radius_x &&
                         bn::abs(walker.prev_y - _state.player_y) <= minimap_radius_y);
            }
        }
        else if(--walker.wait <= 0)
        {
            _start_walker_step(walker);
        }
    }
}

void FieldScene::_start_walker_step(Walker& walker)
{
    // Try the four directions starting from a random one: the first that works is taken.
    int first = _state.random.get_int(4);

    for(int attempt = 0; attempt < 4; ++attempt)
    {
        int direction = (first + attempt) % 4;
        int target_x = walker.x + direction_dx[direction];
        int target_y = walker.y + direction_dy[direction];

        if(bn::abs(target_x - walker.event->x) > walker.event->wander_radius ||
           bn::abs(target_y - walker.event->y) > walker.event->wander_radius)
        {
            continue;       // Too far from home.
        }

        if(! _map->terrain_walkable(target_x, target_y) ||
           _event_at(target_x, target_y) ||                 // Another NPC, a chest, a warp, a sign...
           _walker_blocks(target_x, target_y))              // A walker is leaving that tile.
        {
            continue;
        }

        if(bn::abs(target_x - _state.player_x) <= 1 && bn::abs(target_y - _state.player_y) <= 1)
        {
            continue;       // Keep a tile away from the player: no overlapping sprites, nobody gets trapped.
        }

        // Reserve the destination now, like the player does: nobody else can walk into it.
        walker.prev_x = walker.x;
        walker.prev_y = walker.y;
        walker.x = target_x;
        walker.y = target_y;
        walker.dx = direction_dx[direction];
        walker.dy = direction_dy[direction];
        walker.frames_left = npc_step_frames;

        _minimap_dirty = _minimap_dirty ||
                (bn::abs(target_x - _state.player_x) <= minimap_radius_x &&
                 bn::abs(target_y - _state.player_y) <= minimap_radius_y);
        return;
    }

    walker.wait = npc_min_wait / 2;     // Boxed in: look again soon.
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

const EncounterGroup* FieldScene::_interact()
{
    int target_x = _state.player_x + direction_dx[int(_state.facing)];
    int target_y = _state.player_y + direction_dy[int(_state.facing)];
    const MapEvent* event = _event_at(target_x, target_y);

    if(! event)
    {
        return nullptr;
    }

    // To add a new kind of event (shop, chest, cutscene...): add an EventType and handle it here.
    switch(event->type)
    {

    case EventType::talk:
    case EventType::sign:
        run_talk(_state, _ui, *event);
        break;

    case EventType::inn:
        run_inn(_state, _ui, *event);
        break;

    case EventType::chest:
        {
            // The chest sprite is switched to its open frame by the handler (npc_graphics + 1).
            bn::sprite_ptr* sprite = event->npc_graphics >= 0 ? &_npcs[_npc_sprite_index(*event)] : nullptr;
            open_chest(_state, _ui, *event, sprite);
        }
        break;

    case EventType::spring:
        use_spring(_state, _ui, *event);
        break;

    case EventType::shop:
        // The shop windows cover the top of the screen: the corner windows can't stay under them.
        _hide_hud();
        open_shop(_state, _ui, event->stock, event->pages);
        _show_hud();
        break;

    case EventType::story:
        return _run_story(event->story);

    case EventType::warp:
    case EventType::trigger:
    default:
        break;      // Warps and triggers are started by stepping on them, see _on_step().
    }

    return nullptr;
}

const EncounterGroup* FieldScene::_run_story(StoryId id)
{
    while(id != StoryId::none)
    {
        uint8_t old_progress = _state.story_progress;
        StoryResult result = run_story(id, _state, _ui);

        if(result.step_back)
        {
            // A closed way: back to the tile the player came from, without turning around.
            _state.player_x = uint8_t(_state.player_x - direction_dx[int(_state.facing)]);
            _state.player_y = uint8_t(_state.player_y - direction_dy[int(_state.facing)]);
            _pixel_x = _state.player_x * tile_size;
            _pixel_y = _state.player_y * tile_size;
            _minimap_dirty = true;
        }

        if(result.reload_map)
        {
            // The scene faded the screen out: the map state and the player position may have changed.
            _load_map();
            _update_minimap();
            _ui.update();
            _ui.fade_in();
        }
        else if(_state.story_progress != old_progress)
        {
            _spawn_npcs();      // People come and go with the story.
        }

        if(result.battle)
        {
            _state.story_battle = id;
            return result.battle;
        }

        id = result.next;
    }

    return nullptr;
}

int FieldScene::_npc_sprite_index(const MapEvent& event) const
{
    // _load_map() creates one sprite per event with graphics, in the same order as the events.
    int index = 0;

    for(const MapEvent& other : _map->events)
    {
        if(&other == &event)
        {
            return index;
        }

        if(other.npc_graphics >= 0 && other.active(_state.story_progress))
        {
            ++index;
        }
    }

    BN_ERROR("Event not found in map: ", _map->name);
    return -1;
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

    if(event && event->type == EventType::trigger)
    {
        return _run_story(event->story);
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
