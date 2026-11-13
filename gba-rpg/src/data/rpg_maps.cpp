#include "rpg_maps.h"

#include "bn_assert.h"

namespace rpg
{

namespace
{
    // ---------------------------------------------------------------------------------------------------------------
    // Tile properties. The order must match TileId.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr TileInfo tiles[] = {
        //  symbol  walkable  encounter
        { ' ',      false,    0 },    // void_
        { '.',      true,     1 },    // grass
        { ':',      true,     0 },    // path
        { 'T',      false,    0 },    // tree
        { '~',      false,    0 },    // water
        { '#',      false,    0 },    // wall
        { '^',      false,    0 },    // roof
        { 'D',      false,    0 },    // door (solid for now: make it walkable + add a warp event for interiors)
        { 'S',      false,    0 },    // sign
        { 'M',      false,    0 },    // rock
        { ',',      true,     1 },    // flowers
        { '"',      true,     3 },    // tall_grass
        { '=',      true,     0 },    // bridge
        { '>',      true,     0 },    // gate (use it with a warp event)
        { 'F',      false,    0 },    // spring (use it with a spring event)
        { '_',      true,     2 },    // cave_floor
        { '%',      false,    0 },    // cave_wall
        { 'u',      true,     0 },    // stairs_up (use it with a warp event)
        { 'd',      true,     0 },    // stairs_down (use it with a warp event)
    };

    static_assert(sizeof(tiles) / sizeof(tiles[0]) == int(TileId::count), "Tile table doesn't match TileId");

    [[nodiscard]] constexpr int tile_index_from_symbol(char symbol)
    {
        for(int index = 0; index < int(TileId::count); ++index)
        {
            if(tiles[index].symbol == symbol)
            {
                return index;
            }
        }

        return -1;
    }

    /// Compile time check of a map layout: a typo in the ASCII art becomes a build error.
    [[nodiscard]] constexpr bool valid_layout(bn::span<const bn::string_view> rows)
    {
        if(rows.empty() || rows.size() > max_map_tiles || rows[0].size() > max_map_tiles)
        {
            return false;
        }

        for(const bn::string_view& row : rows)
        {
            if(row.size() != rows[0].size())
            {
                return false;
            }

            for(char symbol : row)
            {
                if(tile_index_from_symbol(symbol) < 0)
                {
                    return false;
                }
            }
        }

        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------
    // Village
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view village_rows[] = {
        //         1111111111222
        //1234567890123456789012 3
        "TTTTTTTTTTTTTTTTTTTTTTTT",    // 0
        "T......................T",    // 1
        "T..^^^^.......^^^^^....T",    // 2
        "T..####.......#####....T",    // 3
        "T..#D##...,...##D##....T",    // 4
        "T...:......,....:....F.T",    // 5
        "T...:::::::::::::......T",    // 6
        "T.......:.......~~~~...T",    // 7
        "T..,....:......~~~~~~..T",    // 8
        "T..^^^..:......~~~~~~..T",    // 9
        "T..###..:.......~~~~...T",    // 10
        "T..#D#..:..S...........T",    // 11
        "T...:...:..............T",    // 12
        "T...:::::....,.....T...T",    // 13
        "T.......:.........TTT..T",    // 14
        "T..,....:..........T...T",    // 15
        "T.......:..............T",    // 16
        "T.......:.....,........T",    // 17
        "T.......:..............T",    // 18
        "TTTTTTTT>TTTTTTTTTTTTTTT",    // 19
    };

    static_assert(valid_layout(village_rows), "Invalid village layout");

    constexpr bn::string_view villager_pages[] = {
        "Welcome to Start Village!",
        "Monsters roam the fields\noutside the south gate.\nBe careful out there.",
        "They say there is a cave in\nthe rocks of the fields.\nTreasure sleeps down there.",
    };

    constexpr bn::string_view elder_pages[] = {
        "Press START to open the menu.\nYou can check your party,\nuse items and save there.",
    };

    constexpr bn::string_view innkeeper_pages[] = {
        "Welcome to the inn.",
    };

    constexpr bn::string_view sign_pages[] = {
        "START VILLAGE\nSouth: the fields",
    };

    constexpr bn::string_view spring_pages[] = {
        "Clear water wells up from\nthe ground. It looks\nrefreshing.",
    };

    constexpr bn::string_view shopkeeper_pages[] = {
        "Welcome to the item shop!",
    };

    constexpr ItemId village_shop_stock[] = {
        ItemId::herb, ItemId::magic_water, ItemId::fire_bomb,
        ItemId::oak_staff, ItemId::leather_armor, ItemId::chain_mail,
    };

    constexpr MapEvent village_events[] = {
        { .type = EventType::talk, .x = 10, .y = 7, .npc_graphics = 0, .pages = villager_pages },
        { .type = EventType::talk, .x = 17, .y = 5, .npc_graphics = 1, .pages = elder_pages },
        { .type = EventType::inn, .x = 5, .y = 12, .npc_graphics = 2, .pages = innkeeper_pages },
        { .type = EventType::sign, .x = 11, .y = 11, .pages = sign_pages },
        { .type = EventType::warp, .x = 8, .y = 19, .warp_map = MapId::field, .warp_x = 15, .warp_y = 11 },
        // Chest with items (npc_graphics 3 = closed chest, 4 = open chest in graphics/npc.bmp).
        { .type = EventType::chest, .x = 11, .y = 13, .npc_graphics = 3,
          .flag = FlagId::chest_village_herbs, .item = ItemId::herb, .item_amount = 3 },
        // Chest with a weapon.
        { .type = EventType::chest, .x = 13, .y = 15, .npc_graphics = 3,
          .flag = FlagId::chest_village_sword, .item = ItemId::bronze_sword, .item_amount = 1 },
        // Healing spring on the 'F' tile: free, but unlike the inn it can't revive fallen members.
        { .type = EventType::spring, .x = 21, .y = 5, .pages = spring_pages, .price = 0 },
        // Item shop (npc_graphics 5 = shopkeeper).
        { .type = EventType::shop, .x = 15, .y = 12, .npc_graphics = 5, .pages = shopkeeper_pages,
          .stock = village_shop_stock },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Field (world map placeholder)
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view field_rows[] = {
        "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM",    // 0
        "M..............\"\"\"\"......~~~~~~M",    // 1
        "M....TT........\"\"\"\".....~~~~~~~M",    // 2
        "M...TTTT.................~~~~~~M",    // 3
        "M....TT.....,.............~~~~.M",    // 4
        "M..............................M",    // 5
        "M..\"\"\"\".............TTT........M",    // 6
        "M..\"\"\"\"\"...........TTTTT.......M",    // 7
        "M...\"\"\".....:::::...TTT........M",    // 8
        "M...........:...:..............M",    // 9
        "M...........:..>:.......\"\"\"\"...M",    // 10
        "M...........:.:::......\"\"\"\"\"\"..M",    // 11
        "M......,....:.:.........\"\"\"\"...M",    // 12
        "M...........:::................M",    // 13
        "M..~~~.......:.................M",    // 14
        "M.~~~~~......:.......,.........M",    // 15
        "M..~~~.......:.................M",    // 16
        "M............:......MMM........M",    // 17
        "M....\"\"\"\"....:.....MMMMM.......M",    // 18
        "M...\"\"\"\"\"\"...:......M>M........M",    // 19
        "M....\"\"\"\"....:.................M",    // 20
        "M............:.....TT..........M",    // 21
        "M............:....TTTT.........M",    // 22
        "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM",    // 23
    };

    static_assert(valid_layout(field_rows), "Invalid field layout");

    constexpr MapEvent field_events[] = {
        { .type = EventType::warp, .x = 15, .y = 10, .warp_map = MapId::village, .warp_x = 8, .warp_y = 18 },
        // Cave entrance: the gate in the rocks.
        { .type = EventType::warp, .x = 21, .y = 19, .warp_map = MapId::cave_1f, .warp_x = 9, .warp_y = 11 },
        // Chest with gold.
        { .type = EventType::chest, .x = 18, .y = 9, .npc_graphics = 3,
          .flag = FlagId::chest_field_gold, .gold = 100 },
        // Chest with a rare weapon.
        { .type = EventType::chest, .x = 24, .y = 5, .npc_graphics = 3,
          .flag = FlagId::chest_field_katana, .item = ItemId::katana, .item_amount = 1 },
    };

    constexpr EncounterGroup field_encounters[] = {
        { { EnemyId::slime }, 1 },
        { { EnemyId::slime, EnemyId::slime }, 2 },
        { { EnemyId::bat }, 1 },
        { { EnemyId::slime, EnemyId::bat }, 2 },
        { { EnemyId::goblin }, 1 },
        { { EnemyId::goblin, EnemyId::slime }, 2 },
        { { EnemyId::bat, EnemyId::slime, EnemyId::bat }, 3 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Cave (3 floors). The stairs are warp events; the walls hold "carved" signs (events on wall tiles).
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view cave_1f_rows[] = {
        "%%%%%%%%%%%%%%%%%%%%",    // 0
        "%%%%%%%%%%%%%%%%%%%%",    // 1
        "%%%%%%%%%%%%%%__d_%%",    // 2
        "%%_______%%%%%____%%",    // 3
        "%%________________%%",    // 4
        "%%________%%%%____%%",    // 5
        "%%________%%%%____%%",    // 6
        "%%%%%%%%%_%%%%%%%%%%",    // 7
        "%%%%%%%%%_%%%%%%%%%%",    // 8
        "%%%%%%%%%_%%%%%%%%%%",    // 9
        "%%%%%%%______%%%%%%%",    // 10
        "%%%%%%%______%%%%%%%",    // 11
        "%%%%%%%__u___%%%%%%%",    // 12
        "%%%%%%%%%%%%%%%%%%%%",    // 13
    };

    static_assert(valid_layout(cave_1f_rows), "Invalid cave_1f layout");

    constexpr bn::string_view cave_2f_rows[] = {
        "%%%%%%%%%%%%%%%%%%%%",    // 0
        "%u____%%%%%%%%%%%%%%",    // 1
        "%______________%%%%%",    // 2
        "%_____%%%%_____%___%",    // 3
        "%%%%%%%%%%_________%",    // 4
        "%%%%%%%%%%_____%___%",    // 5
        "%%%%%%%%%%_____%%%%%",    // 6
        "%%%%%%%%%%_____%%%%%",    // 7
        "%%%%%%%%%%_____%%%%%",    // 8
        "%%%%%%%%%%%%_%%%%%%%",    // 9
        "%%%%%%%%%_________%%",    // 10
        "%%%%%%%%%_________%%",    // 11
        "%%%%%%%%%_______d_%%",    // 12
        "%%%%%%%%%%%%%%%%%%%%",    // 13
    };

    static_assert(valid_layout(cave_2f_rows), "Invalid cave_2f layout");

    constexpr bn::string_view cave_3f_rows[] = {
        "%%%%%%%%%%%%%%%%%%%%",    // 0
        "%%%%%%%%_________%%%",    // 1
        "%%%%%%%%_________%%%",    // 2
        "%%%%%%%%_________%%%",    // 3
        "%%%%%%%%_________%%%",    // 4
        "%%%%%%%%_________%%%",    // 5
        "%%%%%%%%%%%%_%%%%%%%",    // 6
        "%%%%%%%%%%%%_%%%%%%%",    // 7
        "%%%%%%%%%%%%_%%%%%%%",    // 8
        "%____%%%%%%%_%%%%%%%",    // 9
        "%____________%%%%%%%",    // 10
        "%____%%%%%%%%%%%%%%%",    // 11
        "%_u__%%%%%%%%%%%%%%%",    // 12
        "%%%%%%%%%%%%%%%%%%%%",    // 13
    };

    static_assert(valid_layout(cave_3f_rows), "Invalid cave_3f layout");

    constexpr bn::string_view cave_1f_sign_pages[] = {
        "Carved into the wall:\n\"Turn back now, or seek\nthe treasure below.\"",
    };

    constexpr bn::string_view cave_3f_sign_pages[] = {
        "Carved into the wall:\n\"The treasure of the cave\nlies ahead.\"",
    };

    constexpr MapEvent cave_1f_events[] = {
        { .type = EventType::warp, .x = 9, .y = 12, .warp_map = MapId::field, .warp_x = 21, .warp_y = 20 },
        { .type = EventType::warp, .x = 16, .y = 2, .warp_map = MapId::cave_2f, .warp_x = 2, .warp_y = 2 },
        { .type = EventType::sign, .x = 8, .y = 9, .pages = cave_1f_sign_pages },
        { .type = EventType::chest, .x = 3, .y = 3, .npc_graphics = 3,
          .flag = FlagId::chest_cave_1f, .item = ItemId::magic_water, .item_amount = 2 },
    };

    constexpr MapEvent cave_2f_events[] = {
        { .type = EventType::warp, .x = 1, .y = 1, .warp_map = MapId::cave_1f, .warp_x = 16, .warp_y = 3 },
        { .type = EventType::warp, .x = 16, .y = 12, .warp_map = MapId::cave_3f, .warp_x = 2, .warp_y = 11 },
        { .type = EventType::chest, .x = 18, .y = 3, .npc_graphics = 3,
          .flag = FlagId::chest_cave_2f, .gold = 150 },
    };

    constexpr MapEvent cave_3f_events[] = {
        { .type = EventType::warp, .x = 2, .y = 12, .warp_map = MapId::cave_2f, .warp_x = 16, .warp_y = 11 },
        { .type = EventType::sign, .x = 10, .y = 6, .pages = cave_3f_sign_pages },
        { .type = EventType::chest, .x = 12, .y = 1, .npc_graphics = 3,
          .flag = FlagId::chest_cave_3f, .item = ItemId::steel_sword, .item_amount = 1 },
    };

    // Deeper floors bring tougher groups (the skeleton only lives on 3F).
    constexpr EncounterGroup cave_1f_encounters[] = {
        { { EnemyId::slime }, 1 },
        { { EnemyId::slime, EnemyId::slime }, 2 },
        { { EnemyId::bat }, 1 },
        { { EnemyId::bat, EnemyId::bat }, 2 },
        { { EnemyId::slime, EnemyId::bat }, 2 },
    };

    constexpr EncounterGroup cave_2f_encounters[] = {
        { { EnemyId::goblin }, 1 },
        { { EnemyId::goblin, EnemyId::slime }, 2 },
        { { EnemyId::bat, EnemyId::slime, EnemyId::bat }, 3 },
        { { EnemyId::goblin, EnemyId::bat }, 2 },
    };

    constexpr EncounterGroup cave_3f_encounters[] = {
        { { EnemyId::skeleton }, 1 },
        { { EnemyId::goblin, EnemyId::goblin }, 2 },
        { { EnemyId::skeleton, EnemyId::bat }, 2 },
        { { EnemyId::skeleton, EnemyId::goblin, EnemyId::bat }, 3 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Map table. The order must match MapId.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr MapData maps[] = {
        {
            .name = "Start Village",
            .rows = village_rows,
            .events = village_events,
            .border = TileId::tree,
        },
        {
            .name = "Fields",
            .rows = field_rows,
            .events = field_events,
            .encounters = field_encounters,
            .encounter_rate = 20,
            .border = TileId::rock,
        },
        {
            .name = "Cave 1F",
            .rows = cave_1f_rows,
            .events = cave_1f_events,
            .encounters = cave_1f_encounters,
            .encounter_rate = 12,
            .border = TileId::cave_wall,
        },
        {
            .name = "Cave 2F",
            .rows = cave_2f_rows,
            .events = cave_2f_events,
            .encounters = cave_2f_encounters,
            .encounter_rate = 14,
            .border = TileId::cave_wall,
        },
        {
            .name = "Cave 3F",
            .rows = cave_3f_rows,
            .events = cave_3f_events,
            .encounters = cave_3f_encounters,
            .encounter_rate = 16,
            .border = TileId::cave_wall,
        },
    };

    static_assert(sizeof(maps) / sizeof(maps[0]) == int(MapId::count), "Map table doesn't match MapId");

    [[nodiscard]] constexpr bool walkable_tile(bn::span<const bn::string_view> rows, int x, int y)
    {
        if(x < 0 || y < 0 || y >= rows.size() || x >= rows[0].size())
        {
            return false;
        }

        return tiles[tile_index_from_symbol(rows[y][x])].walkable;
    }

    /// Compile time check of the events: a typo in a coordinate becomes a build error instead of a stuck player.
    [[nodiscard]] constexpr bool valid_events()
    {
        for(const MapData& map : maps)
        {
            for(const MapEvent& event : map.events)
            {
                // NPCs and chests stand on walkable tiles (signs and springs are on solid ones, on purpose).
                if(event.npc_graphics >= 0 && ! walkable_tile(map.rows, event.x, event.y))
                {
                    return false;
                }

                if(event.type == EventType::warp)
                {
                    // You must be able to step on it, and to stand where it takes you.
                    if(! walkable_tile(map.rows, event.x, event.y) ||
                       ! walkable_tile(maps[int(event.warp_map)].rows, event.warp_x, event.warp_y))
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    static_assert(valid_events(), "An event is on a solid tile, or a warp leads to a solid tile");
}

const TileInfo& tile_info(TileId id)
{
    BN_ASSERT(id < TileId::count, "Invalid tile id: ", int(id));

    return tiles[int(id)];
}

TileId MapData::tile(int x, int y) const
{
    if(x < 0 || y < 0 || x >= width() || y >= height())
    {
        return border;
    }

    return TileId(tile_index_from_symbol(rows[y][x]));
}

bool MapData::walkable(int x, int y) const
{
    if(x < 0 || y < 0 || x >= width() || y >= height())
    {
        return false;
    }

    if(! tile_info(tile(x, y)).walkable)
    {
        return false;
    }

    // Events with graphics (NPCs, chests, shopkeepers) are solid.
    for(const MapEvent& event : events)
    {
        if(event.x == x && event.y == y && event.npc_graphics >= 0)
        {
            return false;
        }
    }

    return true;
}

const MapData& map_data(MapId id)
{
    BN_ASSERT(id < MapId::count, "Invalid map id: ", int(id));

    return maps[int(id)];
}

}
