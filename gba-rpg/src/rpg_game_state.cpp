#include "rpg_game_state.h"

#include "bn_sram.h"

namespace rpg
{

namespace
{
    constexpr uint32_t save_magic = 0x47505242;     // "BRPG"
    constexpr uint32_t save_version = 3;            // Bump it when SaveData changes: old saves are then ignored.

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

        [[nodiscard]] bool valid() const
        {
            if(magic != save_magic || version != save_version || map >= MapId::count)
            {
                return false;
            }

            // Garbage that happens to have the right header must not crash the game later.
            for(const PartyMember& member : party.members)
            {
                if(! member.valid())
                {
                    return false;
                }
            }

            return true;
        }
    };
}

void GameState::init_new_game()
{
    party.init_new_game();
    flags = EventFlags();
    return_to_start();
}

void GameState::return_to_start()
{
    map = start_map;
    player_x = start_x;
    player_y = start_y;
    facing = Direction::down;
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

    // The map data may have changed since the game was saved (a wall, an NPC or a chest can now be on that
    // tile): never start the player somewhere it can't stand. Same rule as walking and the minimap.
    if(! map_data(state.map).walkable(state.player_x, state.player_y))
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
