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
#include "rpg_story.h"

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
    spring,     //!< A spring or fountain: solid, put a EventType::spring event on it.
    cave_floor, //!< Dungeon floor: walkable, with random battles.
    cave_wall,
    stairs_up,  //!< Walkable, no battles: put a EventType::warp event on it.
    stairs_down,
    ash,        //!< Burnt ground: walkable, no battles.
    rubble,     //!< What is left of a burnt house: solid.
    grave,      //!< Solid: put a EventType::sign event on it.
    altar,      //!< Solid: put a EventType::story or sign event on it.
    lava,       //!< Solid.
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
    cave_1f,
    cave_2f,
    cave_3f,
    berg,       //!< T02: post town.
    fort,       //!< D02: ruined fort on the highway.
    lunaris,    //!< T03: village of fortune tellers.
    ruins,      //!< D03: Star Ruins.
    waterway,   //!< D04: waterway under the gate.
    wasteland,  //!< The burnt plains beyond the gate (second overworld).
    ashbell,    //!< T04: town of survivors.
    canyon,     //!< D05: Searing Canyon.
    dorn,       //!< T05: mining town.
    kazami,     //!< Ruins of the partner's home village.
    cathedral,  //!< D06: Ash Cathedral.
    count
};

enum class EventType : uint8_t
{
    talk,       //!< NPC: shows `pages` when the player presses A in front of it.
    sign,       //!< Like talk, but without sprite (placed over a solid tile like a sign).
    inn,        //!< NPC: shows `pages`, then offers to rest (full restore).
    warp,       //!< Triggered by stepping on the tile: moves to `warp_map` at (warp_x, warp_y).
    chest,      //!< Gives `item` x `item_amount` and/or `gold` once; `flag` remembers that it was opened.
    shop,       //!< NPC: shows `pages` as greeting, then opens an item shop selling `stock`.
    spring,     //!< Healing spring: shows `pages`, then restores HP/MP of the living members for `price` gold.
    story,      //!< Scripted scene `story` (rpg_story.h), started by pressing A in front of it (NPC or solid tile).
    trigger     //!< Scripted scene `story`, started by stepping on the tile.
};

/// How an NPC moves (only for events with graphics, never for chests).
enum class NpcMovement : uint8_t
{
    stand,      //!< Stays where it is.
    wander      //!< Walks around randomly, staying within `wander_radius` tiles of its place in the map data.
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

    // --- Shop (EventType::shop) ---
    bn::span<const ItemId> stock = {};          //!< Items on sale (prices are in the item table).

    // --- Spring (EventType::spring) ---
    uint16_t price = 0;                         //!< Gold per use (0 = free).

    // --- Walking NPCs (events with npc_graphics) ---
    // x/y is where it starts (and the center of its walk). Its position is not saved: it returns there when the
    // map is loaded again.
    NpcMovement movement = NpcMovement::stand;
    uint8_t wander_radius = 2;                  //!< Maximum distance from x/y, in tiles, for NpcMovement::wander.

    // --- Story (see rpg_story.h) ---
    // The event only exists while min_progress <= GameState::story_progress <= max_progress.
    // If several events share a tile, the first one that exists wins.
    uint8_t min_progress = 0;
    uint8_t max_progress = 0xFF;
    StoryId story = StoryId::none;              //!< Scene of EventType::story and EventType::trigger.

    /// talk / sign: if not empty, it replaces `pages` with one set of pages per chapter, indexed by
    /// chapter_index(story_progress). Later chapters reuse the last set.
    bn::span<const bn::span<const bn::string_view>> chapter_pages = {};

    [[nodiscard]] constexpr bool active(int story_progress) const
    {
        return story_progress >= min_progress && story_progress <= max_progress;
    }

    /// The pages of a talk / sign event for the given story progress.
    [[nodiscard]] constexpr bn::span<const bn::string_view> pages_for(int story_progress) const
    {
        if(chapter_pages.empty())
        {
            return pages;
        }

        int chapter = chapter_index(story_progress);
        return chapter_pages[chapter < chapter_pages.size() ? chapter : chapter_pages.size() - 1];
    }
};

constexpr int max_map_tiles = 32;       //!< Maximum map width/height in tiles (a 512x512 pixels background).
constexpr int max_map_npcs = 8;
constexpr int max_map_walkers = 4;     //!< NPCs that walk, per map.

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

    /// Returns true if the tile is inside the map and its terrain can be walked on (NPCs are not considered).
    [[nodiscard]] bool terrain_walkable(int x, int y) const;

    /**
     * Returns true if the player can stand on the given tile *in the map data*: walkable terrain and no NPC or
     * chest that stays in place there (walking NPCs are ignored). Used to check saved positions;
     * FieldScene handles the walking ones. Only the events that exist at `story_progress` count.
     */
    [[nodiscard]] bool walkable(int x, int y, int story_progress) const;
};

/**
 * Returns the map as it is at the given story progress. Most maps never change, but some have several states
 * with their own layout (the home village: peaceful / burnt). All the states of a map have the same size.
 */
[[nodiscard]] const MapData& map_data(MapId id, int story_progress);

/// Where the party wakes up after being defeated: the last safe town the story reached.
struct RespawnPoint
{
    MapId map;
    uint8_t x;
    uint8_t y;
};

[[nodiscard]] RespawnPoint respawn_point(int story_progress);

// Where a new game starts.
constexpr MapId start_map = MapId::village;
constexpr int start_x = 8;
constexpr int start_y = 16;

}

#endif
