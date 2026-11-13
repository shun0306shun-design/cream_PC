/*
 * Map database: tiles, map layouts and events.
 *
 * Maps are written as ASCII art (one character per 16x16 tile) and live in ROM.
 * To add a map: add a MapId, then describe it in src/data/rpg_maps.cpp.
 */

#ifndef RPG_MAPS_H
#define RPG_MAPS_H

#include "bn_span.h"

#include "rpg_enemies.h"
#include "rpg_flags.h"
#include "rpg_items.h"

namespace rpg
{

/// The order must match the tile sheet built by tools/gen_placeholder_assets.py (graphics/field_tiles.bmp).
enum class TileId : uint8_t
{
    void_,
    grass,
    path,
    tree,
    water,
    wall,
    roof,
    door,
    sign,
    rock,
    flowers,
    tall_grass,
    bridge,
    gate,
    count
};

struct TileInfo
{
    char symbol;            //!< Character used in the ASCII map layouts.
    bool walkable;
    uint8_t encounter;      //!< Encounter rate multiplier (0 = safe tile).
};

[[nodiscard]] const TileInfo& tile_info(TileId id);

enum class MapId : uint8_t
{
    village,
    field,
    count
};

enum class EventType : uint8_t
{
    talk,       //!< NPC: shows `pages` when the player presses A in front of it.
    sign,       //!< Like talk, but without sprite (placed over a solid tile like a sign).
    inn,        //!< NPC: shows `pages`, then offers to rest (full restore).
    warp,       //!< Triggered by stepping on the tile: moves to `warp_map` at (warp_x, warp_y).
    chest       //!< Gives `item` x `item_amount` and/or `gold` once; `flag` remembers that it was opened.
};

struct MapEvent
{
    EventType type = EventType::talk;
    uint8_t x = 0;
    uint8_t y = 0;
    int8_t npc_graphics = -1;                   //!< Frame of graphics/npc.bmp, -1 for no sprite.
    bn::span<const bn::string_view> pages = {};  //!< One entry per message box page ('\n' splits lines).
    MapId warp_map = MapId::village;
    uint8_t warp_x = 0;
    uint8_t warp_y = 0;

    // --- Chest (EventType::chest) ---
    // Give it its own FlagId, and a sprite: npc_graphics is the closed chest frame and the next frame
    // (npc_graphics + 1) is the open one.
    FlagId flag = FlagId::none;                 //!< Set when the chest is opened. Every chest needs a different one.
    ItemId item = ItemId::herb;                 //!< Item inside (ignored if item_amount is 0).
    uint8_t item_amount = 0;
    uint16_t gold = 0;                          //!< Gold inside (0 = none). A chest can hold both.
};

constexpr int max_map_tiles = 32;       //!< Maximum map width/height in tiles (a 512x512 pixels background).
constexpr int max_map_npcs = 8;

struct MapData
{
    bn::string_view name;
    bn::span<const bn::string_view> rows;           //!< ASCII layout, all the rows must have the same length.
    bn::span<const MapEvent> events = {};
    bn::span<const EncounterGroup> encounters = {}; //!< Empty = no random battles.
    uint8_t encounter_rate = 0;                     //!< Chance per step out of 256 (scaled by TileInfo::encounter).
    TileId border = TileId::void_;                  //!< Tile shown outside the map.

    [[nodiscard]] constexpr int width() const
    {
        return rows.empty() ? 0 : rows[0].size();
    }

    [[nodiscard]] constexpr int height() const
    {
        return rows.size();
    }

    /// Returns the tile at the given position, or the border tile if it's outside the map.
    [[nodiscard]] TileId tile(int x, int y) const;
};

[[nodiscard]] const MapData& map_data(MapId id);

// Where a new game starts and where the party wakes up after being defeated.
constexpr MapId start_map = MapId::village;
constexpr int start_x = 8;
constexpr int start_y = 16;

}

#endif
