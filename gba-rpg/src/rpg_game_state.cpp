#include "rpg_game_state.h"

#include "bn_sram.h"

#include "rpg_audio.h"

namespace rpg
{

namespace
{
    constexpr uint32_t save_magic = 0x47505242;     // "BRPG"
    constexpr uint32_t save_version = 11;            // Bump it when SaveData changes: old saves are then ignored.

    /// What is stored in SRAM. Must stay trivially copyable.
    struct SaveData
    {
        uint32_t magic = 0;
        uint32_t version = 0;
        Party party;
        EventFlags flags;
        MapId map = start_map;
        uint8_t player_x = 0;
        uint8_t player_y = 0;
        Direction facing = Direction::down;
        uint8_t story_progress = 0;

        [[nodiscard]] bool valid() const
        {
            if(magic != save_magic || version != save_version || map >= MapId::count)
            {
                return false;
            }

            // Garbage that happens to have the right header must not crash the game later.
            return party.valid();
        }
    };

    // Any change to what is saved (a new field, a bigger array...) moves the data of existing saves.
    // If this fails: update the number below AND bump save_version, so old saves are ignored instead of misread.
    // (Adding items or flags doesn't change the size: the bag and EventFlags have fixed capacities.)
    static_assert(sizeof(SaveData) == 180, "SaveData changed: bump save_version and update this size");

    // The sound settings live in SRAM after the saved game (rpg_audio.h).
    static_assert(int(sizeof(SaveData)) <= audio::sram_settings_offset, "SaveData overlaps the sound settings");
}

void GameState::init_new_game()
{
    party.init_new_game();
    flags = EventFlags();
    story_progress = progress::ch1_start;
    story_battle = StoryId::none;
    story_battle_lost = false;
    map = start_map;
    player_x = start_x;
    player_y = start_y;
    facing = Direction::down;
}

namespace
{
    constexpr int respawn_search_radius = 4;    // Tiles around a blocked respawn point that are tried.

    [[nodiscard]] bool tile_safe(MapId map, int x, int y, int story_progress, const EventFlags& flags)
    {
        const MapData& data = map_data(map, story_progress);

        if(! data.walkable(x, y, story_progress, flags))
        {
            return false;
        }

        // Not walled in: there must be somewhere to go from here.
        return data.walkable(x + 1, y, story_progress, flags) || data.walkable(x - 1, y, story_progress, flags) ||
               data.walkable(x, y + 1, story_progress, flags) || data.walkable(x, y - 1, story_progress, flags);
    }

    /// Looks for a safe tile at the given place or around it, closest first. Returns false if there's none.
    [[nodiscard]] bool find_safe_tile(MapId map, int story_progress, const EventFlags& flags, uint8_t& x, uint8_t& y)
    {
        for(int distance = 0; distance <= respawn_search_radius; ++distance)
        {
            for(int dy = -distance; dy <= distance; ++dy)
            {
                for(int dx = -distance; dx <= distance; ++dx)
                {
                    bool on_ring = dx == -distance || dx == distance || dy == -distance || dy == distance;

                    if(on_ring && tile_safe(map, x + dx, y + dy, story_progress, flags))
                    {
                        x = uint8_t(x + dx);
                        y = uint8_t(y + dy);
                        return true;
                    }
                }
            }
        }

        return false;
    }
}

void GameState::return_to_start()
{
    RespawnPoint point = respawn_point(story_progress);
    map = point.map;
    player_x = point.x;
    player_y = point.y;
    facing = Direction::down;

    // The respawn points are checked at build time against the map layouts, but not against the events of every
    // story progress: if someone stands there now, use the closest free tile (towns have no closed areas).
    if(! find_safe_tile(map, story_progress, flags, player_x, player_y))
    {
        // Last resort: where a new game starts.
        map = start_map;
        player_x = start_x;
        player_y = start_y;
        [[maybe_unused]] bool found = find_safe_tile(map, story_progress, flags, player_x, player_y);
    }
}

bool GameState::position_safe() const
{
    return tile_safe(map, player_x, player_y, story_progress, flags);
}

void save_game(const GameState& state)
{
    SaveData data;
    data.magic = save_magic;
    data.version = save_version;
    data.party = state.party;
    data.flags = state.flags;
    data.map = state.map;
    data.player_x = state.player_x;
    data.player_y = state.player_y;
    data.facing = state.facing;
    data.story_progress = state.story_progress;
    bn::sram::write(data);
}

bool load_game(GameState& state)
{
    SaveData data;
    bn::sram::read(data);

    if(! data.valid())
    {
        return false;
    }

    state.party = data.party;
    state.flags = data.flags;
    state.map = data.map;
    state.player_x = data.player_x;
    state.player_y = data.player_y;
    state.facing = data.facing;
    state.story_progress = data.story_progress;
    state.story_battle = StoryId::none;
    state.story_battle_lost = false;

    // Equipment or class data may have changed since the save: keep HP/MP within the current maximums.
    for(PartyMember& member : state.party.active())
    {
        member.clamp_to_stats();
    }

    // A facing outside the enum (a damaged save) would index out of the direction tables.
    if(state.facing > Direction::right)
    {
        state.facing = Direction::down;
    }

    // The map data may have changed since the game was saved (a wall, an NPC or a chest can now be on that tile,
    // or the tile can be walled in), or the position can be outside the map: never start the player somewhere it
    // can't stand or can't leave. It goes back to the town of the current chapter instead of the closest free
    // tile, which could be on the wrong side of a door the story hasn't opened yet.
    if(! state.position_safe())
    {
        state.return_to_start();
    }

    return true;
}

bool has_saved_game()
{
    SaveData data;
    bn::sram::read(data);
    return data.valid();
}

}
