#include "rpg_maps.h"

#include "bn_assert.h"

#include "rpg_text.h"
#include "rpg_names.h"

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
        { 'a',      true,     0 },    // ash
        { 'x',      false,    0 },    // rubble
        { '+',      false,    0 },    // grave (use it with a sign event)
        { 'A',      false,    0 },    // altar (use it with a story or sign event)
        { 'L',      false,    0 },    // lava
        { 'f',      false,    0 },    // fence
        { 'l',      false,    0 },    // lamp
        { 'b',      false,    0 },    // bench
        { 'W',      false,    0 },    // well
        { 'B',      false,    0 },    // barrel
        { 'o',      true,     0 },    // magic_circle (use it with a warp event)
        { 'I',      false,    0 },    // inn_sign (a house wall: put it next to the door)
        { 'G',      false,    0 },    // shop_sign (a house wall: put it next to the door)
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
    // T01: the hero's home village. Two layouts of the same size: peaceful (before the attack, and again once the
    // game is cleared) and burnt (from progress::ch1_ashes on). Both share the event list below.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view village_rows[] = {
        //         1111111111222
        //1234567890123456789012 3
        "TTTTTTTTTTTTTTTTTTTTTTTT",    // 0
        "T......................T",    // 1
        "T..^^^^...W...^^^^^....T",    // 2
        "T..####.......#####....T",    // 3
        "T..#D##...,b..##D##....T",    // 4
        "T...:......,....:....F.T",    // 5
        "T...:::::::::::::......T",    // 6
        "T.......:.......~~~~...T",    // 7
        "T..,....:......~~~~~~..T",    // 8
        "T..^^^..:......~~~~~~..T",    // 9
        "T..###B.:.......~~~~...T",    // 10
        "T..#DI..:..S...........T",    // 11
        "T...:...:.......B......T",    // 12
        "T...:::::....,.....T...T",    // 13
        "T.......:.........TTT..T",    // 14
        "T..,....:..........T...T",    // 15
        "T.......:..............T",    // 16
        "T.......:.....,........T",    // 17
        "T.......:..............T",    // 18
        "TTTTTTfl>lfTTTTTTTTTTTTT",    // 19
    };

    static_assert(valid_layout(village_rows), "Invalid village layout");

    constexpr bn::string_view village_burnt_rows[] = {
        "TTTTTTTTTTTTTTTTTTTTTTTT",    // 0
        "Taaaaaaaaaaaaaaaaaaaaa.T",    // 1
        "TaaxxxxaaaaaaaxxxxxaaaaT",    // 2
        "TaaxxxxaaaaaaaxxxxxaaaaT",    // 3
        "Taaxxxxaaa.aaaxxxxxaaaaT",    // 4
        "Taaa:aaaaaaaaaaa:aaaaFaT",    // 5
        "Taaa:::::::::::::aaaaaaT",    // 6
        "Taaaaaaa:aaaaaaa~~~~aaaT",    // 7
        "Taaaaaaa:a++aaa~~~~~~aaT",    // 8
        "Taaxxxaa:aaaaaa~~~~~~aaT",    // 9
        "Taaxxxaa:aaaaaaa~~~~aaaT",    // 10
        "Taaxxxaa:aaSaaaaaaaaaaaT",    // 11
        "Taaa:aaa:aaaaaaaaaaaaaaT",    // 12
        "Taaa:::::aaaaaaaaaaxaaaT",    // 13
        "Taaaaaaa:aaaaaaaaaxxxaaT",    // 14
        "Taaaaaaa:aaaaaaaaaaxaaaT",    // 15
        "Taaaaaaa:aaaaaaaaaaaaa.T",    // 16
        "Taaaaaaa:aaaaa,aaaaaaaaT",    // 17
        "Taaaaaaa:aaaaaaaaaaaaaaT",    // 18
        "TTTTTTTT>TTTTTTTTTTTTTTT",    // 19
    };

    static_assert(valid_layout(village_burnt_rows), "Invalid burnt village layout");

    // Rebuilt (from progress::ch10_rebuilt on): the old layout again, with the two graves and flowers around them.
    constexpr bn::string_view village_rebuilt_rows[] = {
        "TTTTTTTTTTTTTTTTTTTTTTTT",    // 0
        "T......................T",    // 1
        "T..^^^^...W...^^^^^....T",    // 2
        "T..####.......#####....T",    // 3
        "T..#D##...,b..##D##....T",    // 4
        "T...:......,....:....F.T",    // 5
        "T...:::::::::::::......T",    // 6
        "T.......:.,,....~~~~...T",    // 7
        "T..,....:.++,..~~~~~~..T",    // 8
        "T..^^^..:.,....~~~~~~..T",    // 9
        "T..###B.:.......~~~~...T",    // 10
        "T..#DI..:..S...........T",    // 11
        "T...:...:.......B......T",    // 12
        "T...:::::....,.....T...T",    // 13
        "T.......:.........TTT..T",    // 14
        "T..,....:..........T...T",    // 15
        "T.......:.....,........T",    // 16
        "T.......:.....,........T",    // 17
        "T.......:..............T",    // 18
        "TTTTTTfl>lfTTTTTTTTTTTTT",    // 19
    };

    static_assert(valid_layout(village_rebuilt_rows), "Invalid rebuilt village layout");

    constexpr bn::string_view elder_king_pages[] = {
        RPG_T("Elder: Long live the king.\nNow, the well needs\nmending, sire.",
              "長老「王さま、ばんざい。\n　さて、井戸の修理が\n　残っておりますぞ、陛下。」"),
    };

    constexpr bn::string_view stranger_pages[] = {
        RPG_T("???: I will stay. If I\nflicker, tell me what\nday it is.",
              "？？？「ここにいるよ。\n　もしゆらめいたら、\n　今日が何日か教えて。」"),
    };

    constexpr bn::string_view survivor_rebuilt_pages[] = {
        RPG_T("It isn't the same.\nBut the bread is warm,\nand the door shuts.",
              "元どおりじゃない。\nでもパンは温かいし、\n戸はちゃんと閉まる。"),
    };

    constexpr bn::string_view grave_rebuilt_pages[] = {
        RPG_T("Father and Mother rest\nhere. Flowers grow\naround them now.",
              "父と母が、ここに眠る。\n今は、まわりに\n花が咲いている。"),
    };

    constexpr bn::string_view villager_pages[] = {
        RPG_T("Welcome to " RPG_NAME_VILLAGE " Village!",
              RPG_NAME_VILLAGE_JA "村へようこそ！"),
        RPG_T("Monsters roam the fields\noutside the south gate.\nBe careful out there.",
              "南の門の外には\nまものがうろついている。\n気をつけてね。"),
        RPG_T("The old Watch Cave lies in\nthe rocks to the south.\nNobody goes there anymore.",
              "南の岩山には古い\n見張りの洞窟があるんだ。\nもうだれも近づかないよ。"),
    };

    constexpr bn::string_view elder_pages[] = {
        RPG_T("Press START to open the menu.\nYou can check your party,\nuse items and save there.",
              "STARTでメニューが開く。\n仲間のようすを見たり、道具を\n使ったり、セーブができるぞ。"),
    };

    constexpr bn::string_view mother_pages[] = {
        RPG_T("Mother: Good morning,\n" RPG_NAME_HERO ". Your father is\nby the south gate.",
              "母「おはよう、" RPG_NAME_HERO_JA "。\n　父さんは南の門の\n　そばにいるわよ。」"),
    };

    constexpr bn::string_view innkeeper_pages[] = {
        RPG_T("Welcome to the inn.",
              "宿屋へようこそ。"),
    };

    constexpr bn::string_view sign_pages[] = {
        RPG_T(RPG_NAME_VILLAGE " VILLAGE\nSouth: the fields",
              RPG_NAME_VILLAGE_JA "村\n南: 草原"),
    };

    constexpr bn::string_view spring_pages[] = {
        RPG_T("Clear water wells up from\nthe ground. It looks\nrefreshing.",
              "きれいな水が地面から\nわき出ている。\nつかれがとれそうだ。"),
    };

    constexpr bn::string_view shopkeeper_pages[] = {
        RPG_T("Welcome to the item shop!",
              "道具屋へようこそ！"),
    };

    constexpr bn::string_view grave_pages[] = {
        RPG_T("Two fresh graves.\nFather and Mother\nrest here.",
              "新しい墓が二つ。\n父と母が、ここに\nねむっている。"),
    };

    // NPC lines that change with the chapter: one set per chapter_index() (0 = chapter 1), the last one stays.
    constexpr bn::string_view elder_ch1_pages[] = {
        RPG_T("Elder: The Watch Cave is in\nthe rocks south of here.\nBe careful, " RPG_NAME_HERO ".",
              "長老「見張りの洞窟は\n　南の岩山の中じゃ。\n　気をつけるのじゃぞ、" RPG_NAME_HERO_JA "。」"),
    };

    constexpr bn::string_view elder_ch2_pages[] = {
        RPG_T("Elder: The crest of the\n" RPG_NAME_CULT "... So it was\nthem. " RPG_NAME_TOWN " lies to the east.",
              "長老「" RPG_NAME_CULT_JA "の紋章……\n　やはりやつらであったか。\n　" RPG_NAME_TOWN_JA "は東じゃ。」"),
    };

    constexpr bn::string_view elder_ch3_pages[] = {
        RPG_T("Elder: You travel with a\nfriend now? Good. Ash is\ntoo heavy to carry alone.",
              "長老「仲間ができたか。\n　それはよい。灰は一人で\n　せおうには重すぎる。」"),
    };

    constexpr bn::span<const bn::string_view> elder_chapter_pages[] = {
        elder_ch1_pages, elder_ch2_pages, elder_ch3_pages,
    };

    constexpr bn::string_view survivor_ch1_pages[] = {
        RPG_T("They came at dawn, with\ntorches. I hid in the well.\nI'm sorry... I'm so sorry.",
              "夜明けに、たいまつを持った\nやつらが来た。わしは井戸に\nかくれて……すまん……。"),
    };

    constexpr bn::string_view survivor_ch2_pages[] = {
        RPG_T("We will rebuild " RPG_NAME_VILLAGE ".\nIt won't be the same.\nBut we will.",
              RPG_NAME_VILLAGE_JA "は建て直す。\n元どおりにはならん。\nそれでも、建て直すんだ。"),
    };

    constexpr bn::span<const bn::string_view> survivor_chapter_pages[] = {
        survivor_ch1_pages, survivor_ch2_pages,
    };

    constexpr ItemId village_shop_stock[] = {
        ItemId::herb, ItemId::magic_water, ItemId::fire_bomb, ItemId::leather_armor,
    };

    constexpr uint8_t last_peaceful = progress::ch1_ashes - 1;

    constexpr MapEvent village_events[] = {
        // --- Before the attack ---
        { .type = EventType::talk, .x = 10, .y = 7, .npc_graphics = 0, .pages = villager_pages,
          .movement = NpcMovement::wander, .wander_radius = 3, .max_progress = last_peaceful },
        { .type = EventType::talk, .x = 17, .y = 5, .npc_graphics = 1, .pages = elder_pages,
          .movement = NpcMovement::wander, .wander_radius = 2, .max_progress = last_peaceful },
        { .type = EventType::inn, .x = 5, .y = 12, .npc_graphics = 2, .pages = innkeeper_pages,
          .max_progress = last_peaceful },
        { .type = EventType::shop, .x = 15, .y = 12, .npc_graphics = 5, .pages = shopkeeper_pages,
          .stock = village_shop_stock, .max_progress = last_peaceful },
        { .type = EventType::talk, .x = 5, .y = 5, .npc_graphics = 0, .pages = mother_pages,
          .max_progress = last_peaceful },
        { .type = EventType::story, .x = 9, .y = 17, .npc_graphics = 1,
          .max_progress = last_peaceful, .story = StoryId::ch1_father },
        // Fetching the water starts the attack. It comes before the spring event of the same tile on purpose.
        { .type = EventType::story, .x = 21, .y = 5,
          .min_progress = progress::ch1_errand, .max_progress = progress::ch1_errand, .story = StoryId::ch1_attack },

        // --- After the attack ---
        { .type = EventType::story, .x = 12, .y = 9, .npc_graphics = 1,
          .min_progress = progress::ch1_ashes, .max_progress = progress::ch1_ashes, .story = StoryId::ch1_elder },
        { .type = EventType::talk, .x = 12, .y = 9, .npc_graphics = 1,
          .min_progress = progress::ch1_departed, .max_progress = progress::ch10_ember_out - 1,
          .chapter_pages = elder_chapter_pages },
        { .type = EventType::talk, .x = 5, .y = 7, .npc_graphics = 0,
          .min_progress = progress::ch1_ashes, .max_progress = progress::ch10_rebuilt - 1,
          .chapter_pages = survivor_chapter_pages },
        // Chapter 10: the temptation at the graves (it comes before the grave signs of the same tiles on purpose).
        { .type = EventType::story, .x = 10, .y = 8,
          .min_progress = progress::ch10_home, .max_progress = progress::ch10_home, .story = StoryId::ch10_grave },
        { .type = EventType::story, .x = 11, .y = 8,
          .min_progress = progress::ch10_home, .max_progress = progress::ch10_home, .story = StoryId::ch10_grave },
        { .type = EventType::sign, .x = 10, .y = 8, .pages = grave_pages,
          .min_progress = progress::ch1_ashes, .max_progress = progress::ch10_rebuilt - 1 },
        { .type = EventType::sign, .x = 11, .y = 8, .pages = grave_pages,
          .min_progress = progress::ch1_ashes, .max_progress = progress::ch10_rebuilt - 1 },

        // --- Chapter 10: rebuilding, the coronation, and after the ending ---
        { .type = EventType::story, .x = 12, .y = 9, .npc_graphics = 1,
          .min_progress = progress::ch10_ember_out, .max_progress = progress::game_clear - 1,
          .story = StoryId::ch10_elder },
        // After the ending: the stranger (npc_graphics 11) appears in the square as soon as the hero is crowned.
        { .type = EventType::story, .x = 13, .y = 11, .npc_graphics = 11,
          .min_progress = progress::game_clear, .max_progress = progress::game_clear,
          .story = StoryId::post_stranger },
        { .type = EventType::talk, .x = 13, .y = 11, .npc_graphics = 11, .pages = stranger_pages,
          .min_progress = progress::post_stranger },
        { .type = EventType::talk, .x = 12, .y = 9, .npc_graphics = 1, .pages = elder_king_pages,
          .min_progress = progress::game_clear },
        { .type = EventType::talk, .x = 5, .y = 7, .npc_graphics = 0, .pages = survivor_rebuilt_pages,
          .min_progress = progress::ch10_rebuilt },
        { .type = EventType::sign, .x = 10, .y = 8, .pages = grave_rebuilt_pages,
          .min_progress = progress::ch10_rebuilt },
        { .type = EventType::sign, .x = 11, .y = 8, .pages = grave_rebuilt_pages,
          .min_progress = progress::ch10_rebuilt },
        { .type = EventType::inn, .x = 5, .y = 12, .npc_graphics = 2, .pages = innkeeper_pages,
          .min_progress = progress::ch10_rebuilt },
        { .type = EventType::shop, .x = 15, .y = 12, .npc_graphics = 5, .pages = shopkeeper_pages,
          .stock = village_shop_stock, .min_progress = progress::ch10_rebuilt },

        // --- Always ---
        { .type = EventType::sign, .x = 11, .y = 11, .pages = sign_pages },
        // The gate stays closed until the elder sends the hero away.
        { .type = EventType::trigger, .x = 8, .y = 19,
          .max_progress = progress::ch1_departed - 1, .story = StoryId::ch1_gate },
        { .type = EventType::warp, .x = 8, .y = 19, .warp_map = MapId::field, .warp_x = 15, .warp_y = 11,
          .min_progress = progress::ch1_departed },
        // Chest with items (npc_graphics 3 = closed chest, 4 = open chest in graphics/npc.bmp).
        { .type = EventType::chest, .x = 11, .y = 13, .npc_graphics = 3,
          .flag = FlagId::chest_village_herbs, .item = ItemId::herb, .item_amount = 3 },
        // Chest with armor (the flag keeps its old name: flag ids are never renamed or reordered).
        { .type = EventType::chest, .x = 13, .y = 15, .npc_graphics = 3,
          .flag = FlagId::chest_village_sword, .item = ItemId::leather_armor, .item_amount = 1 },
        // Healing spring on the 'F' tile: free, but unlike the inn it can't revive fallen members.
        { .type = EventType::spring, .x = 21, .y = 5, .pages = spring_pages, .price = 0 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Field (world map placeholder)
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view field_rows[] = {
        "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM",    // 0
        "M........>.....\"\"\"\"......~~~~~~M",    // 1
        "M....TT........\"\"\"\".....~~~~~~~M",    // 2
        "M...TTTT.................~~~~~~M",    // 3
        "M....TT.....,.............~~~~>M",    // 4
        "M..............................M",    // 5
        "M..\"\"\"\".............TTT........M",    // 6
        "M..\"\"\"\"\"...........TTTTT.......M",    // 7
        "M...\"\"\".....:::::...TTT........M",    // 8
        "M...........:...:..............M",    // 9
        "M...........:..>:.......\"\"\"\"...M",    // 10
        "M...........:.:::......\"\"\"\"\"\"..M",    // 11
        "M......,....:.:.........\"\"\"\"...M",    // 12
        "M...........:::::::::::::::::>.M",    // 13
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
        // The road east to the post town is closed until chapter 2.
        { .type = EventType::trigger, .x = 29, .y = 13,
          .max_progress = progress::ch2_start - 1, .story = StoryId::ch2_road },
        { .type = EventType::warp, .x = 29, .y = 13, .warp_map = MapId::berg, .warp_x = 1, .warp_y = 6,
          .min_progress = progress::ch2_start },
        // The mountain path north to the seer's village opens in chapter 3.
        { .type = EventType::trigger, .x = 9, .y = 1,
          .max_progress = progress::ch3_start - 1, .story = StoryId::ch3_seer_road },
        { .type = EventType::warp, .x = 9, .y = 1, .warp_map = MapId::lunaris, .warp_x = 10, .warp_y = 12,
          .min_progress = progress::ch3_start },
        // The sluice by the lake: the way into the waterway, sealed until the Star Sigil is found.
        { .type = EventType::trigger, .x = 30, .y = 4,
          .max_progress = progress::ch3_sigil - 1, .story = StoryId::ch3_sluice },
        { .type = EventType::warp, .x = 30, .y = 4, .warp_map = MapId::waterway, .warp_x = 2, .warp_y = 10,
          .min_progress = progress::ch3_sigil },
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
    // D01: the Watch Cave (3 floors). The stairs are warp events; the walls hold "carved" signs (events on wall tiles).
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
        RPG_T("Carved into the wall:\n\"Watchmen of " RPG_NAME_VILLAGE ",\nkeep the fire burning.\"",
              "かべにほられた文字:\n「" RPG_NAME_VILLAGE_JA "の見張りたちよ、\n　火をたやすな。」"),
    };

    constexpr bn::string_view cave_3f_sign_pages[] = {
        RPG_T("Carved into the wall:\n\"The treasure of the cave\nlies ahead.\"",
              "かべにほられた文字:\n「洞窟のたからは\n　この先にあり。」"),
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
        // The cult scout (npc_graphics 7) blocks the corridor to the treasure room during chapter 1.
        { .type = EventType::story, .x = 12, .y = 8, .npc_graphics = 7,
          .min_progress = progress::ch1_departed, .max_progress = progress::ch2_start - 1,
          .story = StoryId::ch1_scout },
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
    // T02: post town. First real shop and inn; where the partner is met in chapter 2.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view berg_rows[] = {
        "TTTTTTTTTTTTTTTTTTTTTT",    // 0
        "T....................T",    // 1
        "T..^^^^.....^^^^^....T",    // 2
        "T..####.....#####....T",    // 3
        "T..#DI#.....##DG#B...T",    // 4
        "T...:.........:...S..T",    // 5
        ">::::::::::::::::::::>",    // 6
        "T.........:..........T",    // 7
        "T..,......:....^^^...T",    // 8
        "T.........:....###...T",    // 9
        "T...bF....:....#D#...T",    // 10
        "T.........:.....:....T",    // 11
        "T.........:::::::....T",    // 12
        "TTTTTTTTTTTTTTTTTTTTTT",    // 13
    };

    static_assert(valid_layout(berg_rows), "Invalid berg layout");

    constexpr bn::string_view berg_sign_pages[] = {
        RPG_T(RPG_NAME_TOWN " - POST TOWN\nWest: the fields\nEast: the old highway fort",
              "宿場町" RPG_NAME_TOWN_JA "\n西: 草原\n東: 旧街道の砦"),
    };

    constexpr bn::string_view berg_shop_pages[] = {
        RPG_T("Welcome! Travelers get\nthe best prices in " RPG_NAME_TOWN ".",
              "いらっしゃい！ 旅の人には\n" RPG_NAME_TOWN_JA "で一番の値をつけるよ。"),
    };

    constexpr ItemId berg_shop_stock[] = {
        ItemId::herb, ItemId::magic_water, ItemId::fire_bomb,
        ItemId::bronze_sword, ItemId::steel_sword, ItemId::iron_spear, ItemId::leather_armor, ItemId::chain_mail,
    };

    constexpr bn::string_view berg_townsman_ch1_pages[] = {
        RPG_T("This is " RPG_NAME_TOWN ", where the\nhighways meet.",
              "ここは" RPG_NAME_TOWN_JA "。\n街道がまじわる町さ。"),
    };

    constexpr bn::string_view berg_townsman_ch2_pages[] = {
        RPG_T("Men in ash-gray armor took\nthe old fort to the east.\nTheir general is a monster.",
              "灰色のよろいの連中が、東の\n古い砦をのっとったんだ。\nやつらの将軍は化け物だよ。"),
        RPG_T("A bounty hunter with a\nspear was asking about\nhim, too.",
              "槍を持った賞金かせぎも、\nそいつのことを\nたずねていたな。"),
    };

    constexpr bn::string_view berg_townsman_ch3_pages[] = {
        RPG_T("The fort is quiet again.\nThey say the general fled\nnorth, past the " RPG_NAME_GATE ".",
              "砦は静かになったよ。\n将軍は北へ、" RPG_NAME_GATE_JA "の\n向こうへ逃げたそうだ。"),
        RPG_T("The gate is shut to all.\nAsk the seer of " RPG_NAME_SEER_TOWN ", up\nthe path north of the fields.",
              "関所は誰も通れない。\n草原の北の山道の先、\n" RPG_NAME_SEER_TOWN_JA "の占い師に聞きな。"),
    };

    constexpr bn::string_view berg_townsman_ch4_pages[] = {
        RPG_T("You got past the " RPG_NAME_GATE "?\n" RPG_NAME_TOWN " owes you a drink\nwhen you come back.",
              RPG_NAME_GATE_JA "を越えたって？\n帰ってきたら、" RPG_NAME_TOWN_JA "で\n一杯おごるよ。"),
    };

    constexpr bn::span<const bn::string_view> berg_townsman_chapter_pages[] = {
        berg_townsman_ch1_pages, berg_townsman_ch2_pages, berg_townsman_ch3_pages,
        berg_townsman_ch4_pages,
    };

    constexpr MapEvent berg_events[] = {
        { .type = EventType::warp, .x = 0, .y = 6, .warp_map = MapId::field, .warp_x = 28, .warp_y = 13 },
        // The highway opens once the partner has said where the general is.
        { .type = EventType::trigger, .x = 21, .y = 6,
          .max_progress = progress::ch2_rivals - 1, .story = StoryId::ch2_east_gate },
        { .type = EventType::warp, .x = 21, .y = 6, .warp_map = MapId::fort, .warp_x = 8, .warp_y = 11,
          .min_progress = progress::ch2_rivals },
        { .type = EventType::inn, .x = 5, .y = 5, .npc_graphics = 2, .pages = innkeeper_pages },
        { .type = EventType::shop, .x = 15, .y = 5, .npc_graphics = 5, .pages = berg_shop_pages,
          .stock = berg_shop_stock },
        { .type = EventType::sign, .x = 18, .y = 5, .pages = berg_sign_pages },
        { .type = EventType::spring, .x = 5, .y = 10, .pages = spring_pages, .price = 0 },
        { .type = EventType::talk, .x = 7, .y = 9, .npc_graphics = 0,
          .movement = NpcMovement::wander, .wander_radius = 2, .chapter_pages = berg_townsman_chapter_pages },
        // The partner (npc_graphics 6) waits here until the quarrel.
        { .type = EventType::story, .x = 13, .y = 8, .npc_graphics = 6,
          .min_progress = progress::ch2_start, .max_progress = progress::ch2_start,
          .story = StoryId::ch2_partner_town },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // D02: ruined fort on the highway. The general (npc_graphics 8) holds the north-east hall.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view fort_rows[] = {
        "####################",    // 0
        "#______#####_______#",    // 1
        "#______#####_______#",    // 2
        "#__##______________#",    // 3
        "#__##__#####_##____#",    // 4
        "#______#####_##____#",    // 5
        "###_####___####_####",    // 6
        "###_####___####_####",    // 7
        "#_______________####",    // 8
        "#__BB_______##__####",    // 9
        "#__##_______##__####",    // 10
        "#_______________####",    // 11
        "########>###########",    // 12
        "####################",    // 13
    };

    static_assert(valid_layout(fort_rows), "Invalid fort layout");

    constexpr bn::string_view fort_sign_pages[] = {
        RPG_T("A banner hangs here:\na crown wreathed in ash.",
              "旗がかかっている。\n灰をまとった王冠の紋章だ。"),
    };

    constexpr MapEvent fort_events[] = {
        { .type = EventType::warp, .x = 8, .y = 12, .warp_map = MapId::berg, .warp_x = 20, .warp_y = 6 },
        { .type = EventType::sign, .x = 9, .y = 5, .pages = fort_sign_pages },
        { .type = EventType::chest, .x = 8, .y = 6, .npc_graphics = 3,
          .flag = FlagId::chest_fort, .item = ItemId::herb, .item_amount = 3 },
        // Talking to either of them starts the scene. Both leave when chapter 2 ends.
        { .type = EventType::story, .x = 16, .y = 2, .npc_graphics = 6,
          .min_progress = progress::ch2_rivals, .max_progress = progress::ch2_rivals, .story = StoryId::ch2_fort },
        { .type = EventType::story, .x = 17, .y = 1, .npc_graphics = 8,
          .min_progress = progress::ch2_rivals, .max_progress = progress::ch2_allied, .story = StoryId::ch2_fort },
    };

    constexpr EncounterGroup fort_encounters[] = {
        { { EnemyId::cult_soldier }, 1 },
        { { EnemyId::skeleton }, 1 },
        { { EnemyId::cult_soldier, EnemyId::goblin }, 2 },
        { { EnemyId::skeleton, EnemyId::bat }, 2 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // T03: village of fortune tellers, up the mountain path. The seer (npc_graphics 9) joins here in chapter 3.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view lunaris_rows[] = {
        "TTTTTTTTTT>TTTTTTTTTTT",    // 0
        "T.........:..........T",    // 1
        "T..^^^^...:...^^^^...T",    // 2
        "T..####...:...####...T",    // 3
        "T..#DI#...:...#DG#...T",    // 4
        "T...:.....:....:..,..T",    // 5
        "T...:::::::::::::....T",    // 6
        "T.........:..........T",    // 7
        "T..,..F...:....S.....T",    // 8
        "T.........:..........T",    // 9
        "T.........:......,...T",    // 10
        "T.........:..........T",    // 11
        "T.........:..........T",    // 12
        "TTTTTTTTTT>TTTTTTTTTTT",    // 13
    };

    static_assert(valid_layout(lunaris_rows), "Invalid lunaris layout");

    constexpr bn::string_view lunaris_sign_pages[] = {
        RPG_T(RPG_NAME_SEER_TOWN "\nNorth: the Star Ruins\nSouth: the fields",
              RPG_NAME_SEER_TOWN_JA "\n北: 星見の遺跡\n南: 草原"),
    };

    constexpr bn::string_view lunaris_shop_pages[] = {
        RPG_T("Charms, herbs and staves.\nThe stars favor a\nprepared traveler.",
              "お守り、薬草、杖。\n星は、備えある旅人に\n味方します。"),
    };

    constexpr ItemId lunaris_shop_stock[] = {
        ItemId::herb, ItemId::magic_water, ItemId::fire_bomb,
        ItemId::oak_staff, ItemId::steel_sword, ItemId::iron_spear, ItemId::chain_mail,
    };

    constexpr bn::string_view lunaris_villager_ch3_pages[] = {
        RPG_T("The stars over the " RPG_NAME_GATE "\nwent dark a month ago.\n" RPG_NAME_SEER " hasn't slept since.",
              RPG_NAME_GATE_JA "の上の星が消えて\nひと月。" RPG_NAME_SEER_JA "は、\nずっと眠っていないんだ。"),
    };

    constexpr bn::string_view lunaris_villager_ch4_pages[] = {
        RPG_T("So " RPG_NAME_SEER " went with you.\nBring her home, when\nthe sky is clear again.",
              RPG_NAME_SEER_JA "は、あんたたちと\n行ったんだね。空が晴れたら\n連れて帰っておくれ。"),
    };

    // The village can't be reached before chapter 3: the first three sets are never shown.
    constexpr bn::span<const bn::string_view> lunaris_villager_chapter_pages[] = {
        lunaris_villager_ch3_pages, lunaris_villager_ch3_pages, lunaris_villager_ch3_pages,
        lunaris_villager_ch4_pages,
    };

    constexpr MapEvent lunaris_events[] = {
        { .type = EventType::warp, .x = 10, .y = 13, .warp_map = MapId::field, .warp_x = 9, .warp_y = 2 },
        // The ruins open once the seer is in the party.
        { .type = EventType::trigger, .x = 10, .y = 0,
          .max_progress = progress::ch3_seer - 1, .story = StoryId::ch3_ruins_gate },
        { .type = EventType::warp, .x = 10, .y = 0, .warp_map = MapId::ruins, .warp_x = 9, .warp_y = 11,
          .min_progress = progress::ch3_seer },
        { .type = EventType::inn, .x = 5, .y = 5, .npc_graphics = 2, .pages = innkeeper_pages },
        { .type = EventType::shop, .x = 16, .y = 5, .npc_graphics = 5, .pages = lunaris_shop_pages,
          .stock = lunaris_shop_stock },
        { .type = EventType::sign, .x = 15, .y = 8, .pages = lunaris_sign_pages },
        { .type = EventType::spring, .x = 6, .y = 8, .pages = spring_pages, .price = 0 },
        { .type = EventType::talk, .x = 7, .y = 10, .npc_graphics = 0,
          .movement = NpcMovement::wander, .wander_radius = 2, .chapter_pages = lunaris_villager_chapter_pages },
        { .type = EventType::story, .x = 12, .y = 8, .npc_graphics = 9,
          .min_progress = progress::ch3_start, .max_progress = progress::ch3_start,
          .story = StoryId::ch3_seer_village },
        // After the ending: the elder of the seers (npc_graphics 1) comes out to read the stranger's necklace.
        { .type = EventType::story, .x = 12, .y = 8, .npc_graphics = 1,
          .min_progress = progress::post_stranger, .story = StoryId::post_seer_elder },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // D03: Star Ruins. The mirror on the altar ('A') shows the way under the gate.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view ruins_rows[] = {
        "%%%%%%%%%%%%%%%%%%%%",    // 0
        "%%%%%%%%_A_%%%%%%%%%",    // 1
        "%%%%%%%_____%%%%%%%%",    // 2
        "%%%%%%%_____%%%%%%%%",    // 3
        "%%%%%%%%%_%%%%%%%%%%",    // 4
        "%____%%%%_%%%%____%%",    // 5
        "%_________________%%",    // 6
        "%____%%%%_%%%%____%%",    // 7
        "%____%%%%_%%%%%%%%%%",    // 8
        "%%%%%%%%%_%%%%%%%%%%",    // 9
        "%%%%%%%_____%%%%%%%%",    // 10
        "%%%%%%%_____%%%%%%%%",    // 11
        "%%%%%%%%%u%%%%%%%%%%",    // 12
        "%%%%%%%%%%%%%%%%%%%%",    // 13
    };

    static_assert(valid_layout(ruins_rows), "Invalid ruins layout");

    constexpr bn::string_view ruins_mirror_pages[] = {
        RPG_T("The mirror is cracked.\nIts work is done.",
              "鏡はひび割れている。\n役目を終えたのだ。"),
    };

    constexpr MapEvent ruins_events[] = {
        { .type = EventType::warp, .x = 9, .y = 12, .warp_map = MapId::lunaris, .warp_x = 10, .warp_y = 1 },
        { .type = EventType::story, .x = 9, .y = 1,
          .min_progress = progress::ch3_seer, .max_progress = progress::ch3_seer, .story = StoryId::ch3_altar },
        { .type = EventType::sign, .x = 9, .y = 1, .pages = ruins_mirror_pages, .min_progress = progress::ch3_sigil },
        { .type = EventType::chest, .x = 1, .y = 8, .npc_graphics = 3,
          .flag = FlagId::chest_ruins, .item = ItemId::magic_water, .item_amount = 3 },
    };

    constexpr EncounterGroup ruins_encounters[] = {
        { { EnemyId::skeleton, EnemyId::skeleton }, 2 },
        { { EnemyId::skeleton, EnemyId::bat, EnemyId::bat }, 3 },
        { { EnemyId::goblin, EnemyId::skeleton }, 2 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // D04: the old waterway under the gate. Entered from the sluice by the lake of the fields (Star Sigil needed);
    // the stairs in the north-east corner come out beyond the gate.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view waterway_rows[] = {
        "%%%%%%%%%%%%%%%%%%%%",    // 0
        "%%%%%%%%%%%%%%%%_u%%",    // 1
        "%____~~~~____%%%_%%%",    // 2
        "%____~~~~____%%%_%%%",    // 3
        "%____====_________%%",    // 4
        "%____~~~~____%%%%%%%",    // 5
        "%%_%%~~~~%%_%%%%%%%%",    // 6
        "%%_%%~~~~%%_%%%%%%%%",    // 7
        "%____~~~~____%%%%%%%",    // 8
        "%____====____%%%%%%%",    // 9
        "%____~~~~____%%%%%%%",    // 10
        "%_u__~~~~____%%%%%%%",    // 11
        "%%%%%%%%%%%%%%%%%%%%",    // 12
        "%%%%%%%%%%%%%%%%%%%%",    // 13
    };

    static_assert(valid_layout(waterway_rows), "Invalid waterway layout");

    constexpr MapEvent waterway_events[] = {
        { .type = EventType::warp, .x = 2, .y = 11, .warp_map = MapId::field, .warp_x = 30, .warp_y = 5 },
        // The warden (npc_graphics 7) blocks the corridor to the exit until he is beaten.
        { .type = EventType::story, .x = 16, .y = 3, .npc_graphics = 7,
          .min_progress = progress::ch3_sigil, .max_progress = progress::ch4_start - 1,
          .story = StoryId::ch3_warden },
        // Out beyond the gate (only reachable once the warden is gone).
        { .type = EventType::warp, .x = 17, .y = 1, .warp_map = MapId::wasteland, .warp_x = 12, .warp_y = 10 },
        { .type = EventType::chest, .x = 12, .y = 2, .npc_graphics = 3,
          .flag = FlagId::chest_waterway, .item = ItemId::herb, .item_amount = 5 },
    };

    constexpr EncounterGroup waterway_encounters[] = {
        { { EnemyId::cult_soldier, EnemyId::cult_soldier }, 2 },
        { { EnemyId::cult_soldier, EnemyId::bat }, 2 },
        { { EnemyId::skeleton, EnemyId::cult_soldier }, 2 },
        { { EnemyId::slime, EnemyId::slime, EnemyId::slime }, 3 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // The burnt plains beyond the gate: cracked ground ('_', with battles), ash, rubble. Chapter 4 starts here.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view wasteland_rows[] = {
        "%%%%%%%%%%%%%%%%%%%%%%%%",    // 0
        "%____x__________x___:>%%",    // 1
        "%___________xx______:_%%",    // 2
        "%__xx_______________:_%%",    // 3
        "%___________________:_%%",    // 4
        "%_____x_____::::::::::%%",    // 5
        "%___________:____x____%%",    // 6
        "%__>::::::::::________%%",    // 7
        "%___________:_____xx__%%",    // 8
        "%____xx_____:_________%%",    // 9
        "%%%%%%%%%%%%:%%%%%%%%%%%",    // 10
        "%%%%%%%%%%%%d%%%%%%%%%%%",    // 11
        "%%%%%%%%%%%%%%%%%%%%%%%%",    // 12
    };

    static_assert(valid_layout(wasteland_rows), "Invalid wasteland layout");

    constexpr MapEvent wasteland_events[] = {
        { .type = EventType::warp, .x = 12, .y = 11, .warp_map = MapId::waterway, .warp_x = 16, .warp_y = 1 },
        // The only way out of the stairs: the first sight of the burnt world.
        { .type = EventType::trigger, .x = 12, .y = 9,
          .min_progress = progress::ch4_start, .max_progress = progress::ch4_start, .story = StoryId::ch4_wasteland },
        { .type = EventType::warp, .x = 3, .y = 7, .warp_map = MapId::ashbell, .warp_x = 18, .warp_y = 6 },
        { .type = EventType::trigger, .x = 21, .y = 1,
          .max_progress = progress::ch4_truth - 1, .story = StoryId::ch4_canyon_gate },
        { .type = EventType::warp, .x = 21, .y = 1, .warp_map = MapId::canyon, .warp_x = 2, .warp_y = 10,
          .min_progress = progress::ch4_truth },
    };

    constexpr EncounterGroup wasteland_encounters[] = {
        { { EnemyId::cult_knight }, 1 },
        { { EnemyId::skeleton, EnemyId::skeleton }, 2 },
        { { EnemyId::cult_knight, EnemyId::cult_soldier }, 2 },
        { { EnemyId::skeleton, EnemyId::cult_soldier, EnemyId::bat }, 3 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // T04: town of survivors, built on ruins. The chronicler (npc_graphics 1) tells how much of the world burned.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view ashbell_rows[] = {
        "xxxxxxxxxxxxxxxxxxxx",    // 0
        "xaaaaaaaaaaaaaaaaaax",    // 1
        "xa^^^^aaaaxxxaaaaaax",    // 2
        "xa####aaaaxxxaa^^^ax",    // 3
        "xa#DI#aaaaaaaaa###ax",    // 4
        "xaa:aaaaaaaaaaaGD#ax",    // 5
        "xaa::::::::::::::aa>",    // 6
        "xaaaaaaa:aaaaaaaaaax",    // 7
        "xaxxaaaa:aaFaaaxxaax",    // 8
        "xaxxaaaa:aaaaaaaaaax",    // 9
        "xaaaaaaa:aaaaaSaaaax",    // 10
        "xaaaaaaaaaaaaaaaaaax",    // 11
        "xxxxxxxxxxxxxxxxxxxx",    // 12
    };

    static_assert(valid_layout(ashbell_rows), "Invalid ashbell layout");

    constexpr bn::string_view ashbell_sign_pages[] = {
        RPG_T(RPG_NAME_ASH_TOWN "\nBuilt on what was left.",
              RPG_NAME_ASH_TOWN_JA "\n残されたものの上に\n建てられた町。"),
    };

    constexpr bn::string_view ashbell_shop_pages[] = {
        RPG_T("Salvage. All of it.\nTake what you need.",
              "全部、拾いものだ。\n必要なものを\n持っていきな。"),
    };

    constexpr ItemId ashbell_shop_stock[] = {
        ItemId::herb, ItemId::magic_water, ItemId::fire_bomb,
        ItemId::steel_sword, ItemId::iron_spear, ItemId::oak_staff, ItemId::chain_mail,
    };

    constexpr bn::string_view ashbell_spring_pages[] = {
        RPG_T("The water is gray with\nash. It is still water.",
              "水は灰でにごっている。\nそれでも、水だ。"),
    };

    constexpr bn::string_view ashbell_survivor_pages[] = {
        RPG_T("We burn nothing here.\nNot wood. Not candles.\nNot even our dead.",
              "ここでは何も燃やさない。\nまきも、ろうそくも。\n死者さえも。"),
    };

    constexpr bn::string_view ashbell_mother_pages[] = {
        RPG_T("My son asked what a tree\nwas. I drew him one.\nHe didn't believe me.",
              "息子に、木って何？と\n聞かれてね。絵に描いたけど\n信じてくれなかったよ。"),
    };

    constexpr bn::string_view ashbell_chronicler_pages[] = {
        RPG_T("Chronicler: I keep the map\nso someone remembers\nwhat was there.",
              "記録者「地図を残すのは、\n　そこに何があったか、\n　誰かが覚えているためさ。」"),
        RPG_T("Chronicler: The canyon is\nto the northeast. Past it,\nthe tower of flame.",
              "記録者「峡谷は北東だよ。\n　その先に、\n　炎の塔がある。」"),
    };

    constexpr MapEvent ashbell_events[] = {
        { .type = EventType::warp, .x = 19, .y = 6, .warp_map = MapId::wasteland, .warp_x = 4, .warp_y = 7 },
        { .type = EventType::inn, .x = 4, .y = 5, .npc_graphics = 2, .pages = innkeeper_pages },
        { .type = EventType::shop, .x = 14, .y = 5, .npc_graphics = 5, .pages = ashbell_shop_pages,
          .stock = ashbell_shop_stock },
        { .type = EventType::sign, .x = 14, .y = 10, .pages = ashbell_sign_pages },
        { .type = EventType::spring, .x = 11, .y = 8, .pages = ashbell_spring_pages, .price = 0 },
        { .type = EventType::talk, .x = 5, .y = 9, .npc_graphics = 0, .pages = ashbell_survivor_pages,
          .movement = NpcMovement::wander, .wander_radius = 2 },
        { .type = EventType::talk, .x = 14, .y = 2, .npc_graphics = 0, .pages = ashbell_mother_pages },
        { .type = EventType::story, .x = 10, .y = 8, .npc_graphics = 1,
          .max_progress = progress::ch4_truth - 1, .story = StoryId::ch4_chronicler },
        { .type = EventType::talk, .x = 10, .y = 8, .npc_graphics = 1, .pages = ashbell_chronicler_pages,
          .min_progress = progress::ch4_truth },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // D05: Searing Canyon. Stone bridges over lava ('L'); the wyrm guards the corridor to the far rim.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view canyon_rows[] = {
        "%%%%%%%%%%%%%%%%%%%%",    // 0
        "%%%%%%%%%%%%%%%_u_%%",    // 1
        "%___LLLL_____%%_%%%%",    // 2
        "%___LLLL_____%%_%%%%",    // 3
        "%___====__________%%",    // 4
        "%___LLLL_____%%%%%%%",    // 5
        "%%_%LLLL%%_%%%%%%%%%",    // 6
        "%%_%LLLL%%_%%%%%%%%%",    // 7
        "%___LLLL_____%%%%%%%",    // 8
        "%___====_____%%%%%%%",    // 9
        "%___LLLL_____%%%%%%%",    // 10
        "%_d_LLLL_____%%%%%%%",    // 11
        "%%%%%%%%%%%%%%%%%%%%",    // 12
        "%%%%%%%%%%%%%%%%%%%%",    // 13
    };

    static_assert(valid_layout(canyon_rows), "Invalid canyon layout");

    constexpr MapEvent canyon_events[] = {
        { .type = EventType::warp, .x = 2, .y = 11, .warp_map = MapId::wasteland, .warp_x = 20, .warp_y = 1 },
        // The wyrm (npc_graphics 10) blocks the corridor to the exit until it is beaten.
        { .type = EventType::story, .x = 15, .y = 3, .npc_graphics = 10,
          .min_progress = progress::ch4_truth, .max_progress = progress::ch5_start - 1,
          .story = StoryId::ch4_wyrm },
        // Out to the mining town (only reachable once the wyrm is gone).
        { .type = EventType::warp, .x = 16, .y = 1, .warp_map = MapId::dorn, .warp_x = 1, .warp_y = 6 },
        { .type = EventType::chest, .x = 12, .y = 2, .npc_graphics = 3,
          .flag = FlagId::chest_canyon, .item = ItemId::magic_water, .item_amount = 3 },
    };

    constexpr EncounterGroup canyon_encounters[] = {
        { { EnemyId::cult_knight, EnemyId::cult_knight }, 2 },
        { { EnemyId::cult_knight, EnemyId::skeleton, EnemyId::bat }, 3 },
        { { EnemyId::cult_knight, EnemyId::cult_soldier }, 2 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // T05: mining town at the far end of the canyon. The former cult priest (npc_graphics 7) waits here.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view dorn_rows[] = {
        "%%%%%%%%%%>%%%%%%%%%%%",    // 0
        "%_________:__________%",    // 1
        "%__^^^^___:___^^^^___%",    // 2
        "%__####___:___####___%",    // 3
        "%__#DI#___:___#DG#___%",    // 4
        "%___:_____:____:_____%",    // 5
        ">::::::::::::::::::::>",    // 6
        "%_________:__________%",    // 7
        "%___F_____:____S_____%",    // 8
        "%_________:__xx______%",    // 9
        "%____________xx______%",    // 10
        "%____________________%",    // 11
        "%%%%%%%%%%>%%%%%%%%%%%",    // 12
    };

    static_assert(valid_layout(dorn_rows), "Invalid dorn layout");

    constexpr bn::string_view dorn_sign_pages[] = {
        RPG_T(RPG_NAME_MINE_TOWN " - MINING TOWN\nNorth: ruins\nEast: the Ash Cathedral",
              "鉱山町" RPG_NAME_MINE_TOWN_JA "\n北: 廃墟\n東: 灰の聖堂"),
    };

    constexpr bn::string_view dorn_shop_pages[] = {
        RPG_T("Picks, blades, bandages.\nAll the mine has left.",
              "つるはし、刃物、包帯。\n鉱山に残ったのは\nこれで全部さ。"),
    };

    constexpr bn::string_view dorn_miner_pages[] = {
        RPG_T("We sell ore to the cult.\nJudge us when your town\nis still standing.",
              "鉱石は教団に売っている。\n責めるなら、あんたの町が\n残ってからにしな。"),
    };

    constexpr MapEvent dorn_events[] = {
        { .type = EventType::warp, .x = 0, .y = 6, .warp_map = MapId::canyon, .warp_x = 15, .warp_y = 1 },
        // E501: the first steps into the town (every tile next to the arrival tile).
        { .type = EventType::trigger, .x = 2, .y = 6,
          .max_progress = progress::ch5_dorn - 1, .story = StoryId::ch5_dorn },
        { .type = EventType::trigger, .x = 1, .y = 5,
          .max_progress = progress::ch5_dorn - 1, .story = StoryId::ch5_dorn },
        { .type = EventType::trigger, .x = 1, .y = 7,
          .max_progress = progress::ch5_dorn - 1, .story = StoryId::ch5_dorn },
        // E502: behind the tavern (the north-east house).
        { .type = EventType::trigger, .x = 15, .y = 1,
          .min_progress = progress::ch5_dorn, .max_progress = progress::ch5_deserter - 1,
          .story = StoryId::ch5_deserter },
        { .type = EventType::trigger, .x = 16, .y = 1,
          .min_progress = progress::ch5_dorn, .max_progress = progress::ch5_deserter - 1,
          .story = StoryId::ch5_deserter },
        // The abandoned mine (D06) opens when the defector shows the way.
        { .type = EventType::trigger, .x = 10, .y = 0,
          .max_progress = progress::ch5_deserter - 1, .story = StoryId::ch5_mine_gate },
        { .type = EventType::warp, .x = 10, .y = 0, .warp_map = MapId::mine, .warp_x = 7, .warp_y = 8,
          .min_progress = progress::ch5_deserter },
        // The main road to the cathedral never opens: the way in is through the mine and the ruins.
        { .type = EventType::trigger, .x = 21, .y = 6, .story = StoryId::ch5_east_gate },
        // The track south to the valley of the refugees opens in chapter 7.
        { .type = EventType::trigger, .x = 10, .y = 12,
          .max_progress = progress::ch7_start - 1, .story = StoryId::ch7_south_gate },
        { .type = EventType::warp, .x = 10, .y = 12, .warp_map = MapId::refuge, .warp_x = 9, .warp_y = 1,
          .min_progress = progress::ch7_start },
        { .type = EventType::inn, .x = 5, .y = 5, .npc_graphics = 2, .pages = innkeeper_pages },
        { .type = EventType::shop, .x = 16, .y = 5, .npc_graphics = 5, .pages = dorn_shop_pages,
          .stock = ashbell_shop_stock },
        { .type = EventType::sign, .x = 15, .y = 8, .pages = dorn_sign_pages },
        { .type = EventType::spring, .x = 4, .y = 8, .pages = ashbell_spring_pages, .price = 0 },
        { .type = EventType::talk, .x = 7, .y = 9, .npc_graphics = 0, .pages = dorn_miner_pages,
          .movement = NpcMovement::wander, .wander_radius = 2 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // D06: the abandoned mine north of the mining town. The defector knows it: it comes out at the ruins of the
    // partner's village, behind the cathedral. The overseer (npc_graphics 7) blocks the last gallery.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view mine_rows[] = {
        "%%%%%%%%%%%%%%%%%%%%",    // 0
        "%%%%%%%%%%%%%%%_u_%%",    // 1
        "%_____%%%%%%%%%_%%%%",    // 2
        "%_%%%_%%%____%%_%%%%",    // 3
        "%_%%%_%%%_%%_%%_%%%%",    // 4
        "%_%%%_____%%____%%%%",    // 5
        "%_%%%%%%%%%%%%%%%%%%",    // 6
        "%_______%%%%%%%%%%%%",    // 7
        "%%%%%%%_%%%%%%%%%%%%",    // 8
        "%%%%%%%d%%%%%%%%%%%%",    // 9
        "%%%%%%%%%%%%%%%%%%%%",    // 10
    };

    static_assert(valid_layout(mine_rows), "Invalid mine layout");

    constexpr MapEvent mine_events[] = {
        { .type = EventType::warp, .x = 7, .y = 9, .warp_map = MapId::dorn, .warp_x = 10, .warp_y = 1 },
        { .type = EventType::warp, .x = 16, .y = 1, .warp_map = MapId::kazami, .warp_x = 9, .warp_y = 9 },
        // E503.
        { .type = EventType::story, .x = 15, .y = 3, .npc_graphics = 7,
          .min_progress = progress::ch5_deserter, .max_progress = progress::ch5_mine - 1,
          .story = StoryId::ch5_overseer },
        { .type = EventType::chest, .x = 17, .y = 1, .npc_graphics = 3,
          .flag = FlagId::chest_mine, .item = ItemId::magic_water, .item_amount = 3 },
    };

    constexpr EncounterGroup mine_encounters[] = {
        { { EnemyId::cult_knight }, 1 },
        { { EnemyId::skeleton, EnemyId::skeleton, EnemyId::bat }, 3 },
        { { EnemyId::cult_knight, EnemyId::skeleton }, 2 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Ruins of the partner's home village: no battles, one grave ('+').
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view kazami_rows[] = {
        "xxxxxxxxx>xxxxxxxxxx",    // 0
        "xaaaaaaaaaaaaaaaaaax",    // 1
        "xaaxxxaaaaaaaxxxaaax",    // 2
        "xaaxxxaaa+aaaxxxaaax",    // 3
        "xaaaaaaaaaaaaaaaaaax",    // 4
        "xaaaaaaaa:aaaaaaaaax",    // 5
        "xaxxaaaaa:aaaaaxxaax",    // 6
        "xaxxaaaaa:aaaaaxxaax",    // 7
        "xaaaaaaaa:aaaaaaaaax",    // 8
        "xaaaaaaaa:aaaaaaaaax",    // 9
        "xxxxxxxxx>xxxxxxxxxx",    // 10
    };

    static_assert(valid_layout(kazami_rows), "Invalid kazami layout");

    constexpr bn::string_view kazami_grave_pages[] = {
        RPG_T(RPG_NAME_PARTNER_HOME ".\nThe marker is clean now.\nSomeone was here.",
              RPG_NAME_PARTNER_HOME_JA "。\n墓標はきれいになっている。\n誰かが来たのだ。"),
    };

    constexpr MapEvent kazami_events[] = {
        { .type = EventType::warp, .x = 9, .y = 10, .warp_map = MapId::mine, .warp_x = 15, .warp_y = 1 },
        // E504: the first steps into the ruins (every tile next to the arrival tile).
        { .type = EventType::trigger, .x = 9, .y = 8,
          .max_progress = progress::ch5_kazami - 1, .story = StoryId::ch5_kazami },
        { .type = EventType::trigger, .x = 8, .y = 9,
          .max_progress = progress::ch5_kazami - 1, .story = StoryId::ch5_kazami },
        { .type = EventType::trigger, .x = 10, .y = 9,
          .max_progress = progress::ch5_kazami - 1, .story = StoryId::ch5_kazami },
        // E505: the grave marker.
        { .type = EventType::story, .x = 9, .y = 3,
          .max_progress = progress::ch5_split - 1, .story = StoryId::ch5_confession },
        { .type = EventType::sign, .x = 9, .y = 3, .pages = kazami_grave_pages, .min_progress = progress::ch5_split },
        // The way to the cathedral, behind the village.
        { .type = EventType::trigger, .x = 9, .y = 0,
          .max_progress = progress::ch5_split - 1, .story = StoryId::ch5_cathedral_gate },
        { .type = EventType::warp, .x = 9, .y = 0, .warp_map = MapId::cathedral, .warp_x = 9, .warp_y = 10,
          .min_progress = progress::ch5_split },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // D06: Ash Cathedral. General 1 (npc_graphics 8) blocks the corridor to the pyre.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view cathedral_rows[] = {
        "####################",    // 0
        "########_Ao#########",    // 1
        "#######_____########",    // 2
        "#######_____########",    // 3
        "#########_##########",    // 4
        "#___#####_#####____#",    // 5
        "#__________________#",    // 6
        "#___#####_#####____#",    // 7
        "#########_##########",    // 8
        "#######_____########",    // 9
        "#######_____########",    // 10
        "#########>##########",    // 11
        "####################",    // 12
    };

    static_assert(valid_layout(cathedral_rows), "Invalid cathedral layout");

    constexpr bn::string_view cathedral_pyre_pages[] = {
        RPG_T("A cold pyre.\nThe ash in it is\nstill warm.",
              "冷えた火葬台。\n中の灰は、\nまだあたたかい。"),
    };

    constexpr MapEvent cathedral_events[] = {
        { .type = EventType::warp, .x = 9, .y = 11, .warp_map = MapId::kazami, .warp_x = 9, .warp_y = 1 },
        // E506: General Zelma (npc_graphics 8) blocks the corridor to the pyre until she flees.
        { .type = EventType::story, .x = 9, .y = 4, .npc_graphics = 8,
          .min_progress = progress::ch5_split, .max_progress = progress::ch6_start - 1,
          .story = StoryId::ch5_zelma },
        { .type = EventType::sign, .x = 9, .y = 1, .pages = cathedral_pyre_pages },
        // Behind the pyre: the pilgrim road to the capital, until the capital falls.
        { .type = EventType::warp, .x = 10, .y = 1, .warp_map = MapId::grancel, .warp_x = 8, .warp_y = 12,
          .max_progress = progress::ch6_collapse - 1 },
        { .type = EventType::trigger, .x = 10, .y = 1,
          .min_progress = progress::ch6_collapse, .story = StoryId::ch6_road_lost },
        { .type = EventType::chest, .x = 17, .y = 5, .npc_graphics = 3,
          .flag = FlagId::chest_cathedral, .item = ItemId::herb, .item_amount = 5 },
    };

    constexpr EncounterGroup cathedral_encounters[] = {
        { { EnemyId::ash_zealot }, 1 },
        { { EnemyId::ash_zealot, EnemyId::cult_knight }, 2 },
        { { EnemyId::cult_knight, EnemyId::cult_knight }, 2 },
        { { EnemyId::ash_zealot, EnemyId::skeleton, EnemyId::skeleton }, 3 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // T06: the royal capital. Peaceful until the siege, then burning: same size, the streets ('_') have battles
    // and the east gate is gone. Both layouts share the event list.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view grancel_rows[] = {
        "########################",    // 0
        "#....^^^^^^....^^^^....#",    // 1
        "#....######....####....#",    // 2
        "#....##DD##....#D##....#",    // 3
        "#.....l::l......:......#",    // 4
        "#..F...::.......:...S..#",    // 5
        "#::::::::::::::::::::::>",    // 6
        "#.....l::l.............#",    // 7
        "#.^^^^.::...^^^^.......#",    // 8
        "#.####.::...####...,...#",    // 9
        "#.#DI#.::...#DG#.......#",    // 10
        "#..:...::....:.........#",    // 11
        "#..:::::::::::.........#",    // 12
        "########>###############",    // 13
    };

    static_assert(valid_layout(grancel_rows), "Invalid grancel layout");

    constexpr bn::string_view grancel_burning_rows[] = {
        "########################",    // 0
        "#aaaa^^^^^^aaaaxxxxaaaa#",    // 1
        "#aaaa######aaaaxxxxaaaa#",    // 2
        "#aaaa##DD##aaaaxxxxaaaa#",    // 3
        "#aaaaaa__aaaaaaa_aaaaaa#",    // 4
        "#aaFaaa__aaaaaaa_aaaSaa#",    // 5
        "#______________________x",    // 6
        "#aaaaaa__aaaaaaaaaaaaaa#",    // 7
        "#axxxxa__aaaxxxxaaaaaaa#",    // 8
        "#axxxxa__aaaxxxxaaaaaaa#",    // 9
        "#axxxxa__aaaxxxxaaaaaaa#",    // 10
        "#aa_aaa__aaaa_aaaaaaaaa#",    // 11
        "#aa___________aaaaaaaaa#",    // 12
        "########>###############",    // 13
    };

    static_assert(valid_layout(grancel_burning_rows), "Invalid burning grancel layout");

    constexpr bn::string_view grancel_sign_pages[] = {
        RPG_T(RPG_NAME_CAPITAL " - ROYAL CAPITAL\nNorth: the palace\nEast: the great wall",
              "王都" RPG_NAME_CAPITAL_JA "\n北: 宮殿\n東: 大城壁"),
    };

    constexpr bn::string_view grancel_shop_pages[] = {
        RPG_T("The royal armory is open\nto anyone who will stand\non the wall.",
              "城壁に立つ者になら、\n王家の武器庫は\n誰にでも開かれている。"),
    };

    constexpr bn::string_view grancel_citizen_pages[] = {
        RPG_T("The king says the wall\nhas never fallen.\nIt has never been tried.",
              "王さまは、城壁は一度も\n落ちたことがないと言う。\n攻められたこともないのさ。"),
    };

    constexpr uint8_t last_before_siege = progress::ch6_collapse - 1;    // The city stands until the castle falls.

    constexpr MapEvent grancel_events[] = {
        { .type = EventType::warp, .x = 8, .y = 13, .warp_map = MapId::cathedral, .warp_x = 10, .warp_y = 2 },
        { .type = EventType::spring, .x = 3, .y = 5, .pages = spring_pages, .price = 0 },
        { .type = EventType::sign, .x = 20, .y = 5, .pages = grancel_sign_pages },

        // --- Under siege (until the castle falls) ---
        // E601: the first steps into the city (every tile next to the arrival tile).
        { .type = EventType::trigger, .x = 8, .y = 11,
          .max_progress = progress::ch6_audience - 1, .story = StoryId::ch6_audience },
        { .type = EventType::trigger, .x = 7, .y = 12,
          .max_progress = progress::ch6_audience - 1, .story = StoryId::ch6_audience },
        { .type = EventType::trigger, .x = 9, .y = 12,
          .max_progress = progress::ch6_audience - 1, .story = StoryId::ch6_audience },
        // E602 / E603: the innkeeper runs the eve and the morning of the battle.
        { .type = EventType::story, .x = 4, .y = 11, .npc_graphics = 2,
          .max_progress = last_before_siege, .story = StoryId::ch6_inn },
        { .type = EventType::shop, .x = 14, .y = 11, .npc_graphics = 5, .pages = grancel_shop_pages,
          .stock = ashbell_shop_stock, .max_progress = last_before_siege },
        { .type = EventType::talk, .x = 18, .y = 9, .npc_graphics = 0, .pages = grancel_citizen_pages,
          .movement = NpcMovement::wander, .wander_radius = 2, .max_progress = last_before_siege },
        // E602: the companions, out in the city on the eve (npc_graphics 6 partner, 9 seer, 12 defector).
        { .type = EventType::story, .x = 11, .y = 7, .npc_graphics = 6,
          .min_progress = progress::ch6_eve, .max_progress = progress::ch6_eve, .story = StoryId::ch6_eve_partner },
        { .type = EventType::story, .x = 5, .y = 4, .npc_graphics = 9,
          .min_progress = progress::ch6_eve, .max_progress = progress::ch6_eve, .story = StoryId::ch6_eve_seer },
        { .type = EventType::story, .x = 18, .y = 7, .npc_graphics = 12,
          .min_progress = progress::ch6_eve, .max_progress = progress::ch6_eve, .story = StoryId::ch6_eve_defector },
        // The palace doors (the 'D' tiles): the way into the royal castle once the gate is held.
        { .type = EventType::story, .x = 7, .y = 3, .max_progress = last_before_siege, .story = StoryId::ch6_palace },
        { .type = EventType::story, .x = 8, .y = 3, .max_progress = last_before_siege, .story = StoryId::ch6_palace },
        { .type = EventType::trigger, .x = 23, .y = 6,
          .max_progress = last_before_siege, .story = StoryId::ch6_great_gate },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // D08: the royal castle of the capital: the throne room. General Bardos (npc_graphics 8) stands over the king.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view throne_rows[] = {
        "####################",    // 0
        "########_A_#########",    // 1
        "#######_____########",    // 2
        "#######_____########",    // 3
        "#########_##########",    // 4
        "#########_##########",    // 5
        "######_______#######",    // 6
        "######_______#######",    // 7
        "#########_##########",    // 8
        "#########>##########",    // 9
        "####################",    // 10
    };

    static_assert(valid_layout(throne_rows), "Invalid throne layout");

    constexpr bn::string_view throne_pages[] = {
        RPG_T("The throne of " RPG_NAME_CAPITAL ".",
              RPG_NAME_CAPITAL_JA "の玉座だ。"),
    };

    constexpr MapEvent throne_events[] = {
        { .type = EventType::warp, .x = 9, .y = 9, .warp_map = MapId::grancel, .warp_x = 8, .warp_y = 4 },
        // E604.
        { .type = EventType::story, .x = 9, .y = 4, .npc_graphics = 8,
          .min_progress = progress::ch6_gate, .max_progress = last_before_siege, .story = StoryId::ch6_bardos },
        { .type = EventType::sign, .x = 9, .y = 1, .pages = throne_pages },
    };

    constexpr EncounterGroup grancel_encounters[] = {
        { { EnemyId::ash_zealot, EnemyId::cult_knight }, 2 },
        { { EnemyId::cult_knight, EnemyId::cult_knight }, 2 },
        { { EnemyId::ash_zealot, EnemyId::ash_zealot }, 2 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // T07: the valley of the refugees, south of the mining town. The fire never reached it: grass and trees again.
    // The lantern shrine is the altar ('A').
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view refuge_rows[] = {
        "TTTTTTTTT>TTTTTTTTTT",    // 0
        "T........:.........T",    // 1
        "T.^^^^...:....A....T",    // 2
        "T.####...:....:....T",    // 3
        "T.#DI#...:....:....T",    // 4
        "T..:.....:....:....T",    // 5
        "T..::::::::::::::::>",    // 6
        "T........:.........T",    // 7
        "T..,..F..:...,.....T",    // 8
        "T........:....S....T",    // 9
        "T........:.........T",    // 10
        "T..................T",    // 11
        "TTTTTTTTTTTTTTTTTTTT",    // 12
    };

    static_assert(valid_layout(refuge_rows), "Invalid refuge layout");

    constexpr bn::string_view refuge_sign_pages[] = {
        RPG_T(RPG_NAME_REFUGE "\nKeep a lamp lit for\nthose still on the road.",
              RPG_NAME_REFUGE_JA "\nまだ道の途中にいる者の\nために、灯火をひとつ。"),
    };

    constexpr bn::string_view refuge_shop_pages[] = {
        RPG_T("Whatever we carried out\nof " RPG_NAME_CAPITAL ". Take what\nhelps.",
              RPG_NAME_CAPITAL_JA "から持ち出せた\nものばかりさ。\n役に立つものを持っていきな。"),
    };

    constexpr bn::string_view refuge_shrine_before_pages[] = {
        RPG_T("A campfire. People warm\ntheir hands, and say\nlittle.",
              "焚き火だ。人びとが\n手をあたため、\nだまって火を見ている。"),
    };

    constexpr bn::string_view refuge_shrine_after_pages[] = {
        RPG_T("The fire burns on.",
              "火は、燃えつづけている。"),
    };

    constexpr MapEvent refuge_events[] = {
        { .type = EventType::warp, .x = 9, .y = 0, .warp_map = MapId::dorn, .warp_x = 10, .warp_y = 11 },
        // The road east (the central highway): not while the hero is alone.
        { .type = EventType::trigger, .x = 19, .y = 6,
          .max_progress = progress::ch7_seer - 1, .story = StoryId::ch7_east_gate },
        { .type = EventType::warp, .x = 19, .y = 6, .warp_map = MapId::pass, .warp_x = 2, .warp_y = 1,
          .min_progress = progress::ch7_seer },
        { .type = EventType::inn, .x = 4, .y = 5, .npc_graphics = 2, .pages = innkeeper_pages },
        { .type = EventType::shop, .x = 12, .y = 8, .npc_graphics = 5, .pages = refuge_shop_pages,
          .stock = ashbell_shop_stock },
        { .type = EventType::sign, .x = 14, .y = 9, .pages = refuge_sign_pages },
        { .type = EventType::spring, .x = 6, .y = 8, .pages = spring_pages, .price = 0 },
        // Rico, the child who pulled the hero out of the river (E701).
        { .type = EventType::story, .x = 5, .y = 10, .npc_graphics = 13,
          .movement = NpcMovement::wander, .wander_radius = 2, .story = StoryId::ch7_rico },
        // E702: the seer (npc_graphics 9) tends the wounded by the big house until she rejoins.
        { .type = EventType::story, .x = 6, .y = 6, .npc_graphics = 9,
          .min_progress = progress::ch7_awake, .max_progress = progress::ch7_seer - 1,
          .story = StoryId::ch7_seer },
        // E805: coming back with both generals dead (every tile next to the two arrival tiles).
        { .type = EventType::trigger, .x = 8, .y = 1,
          .min_progress = progress::ch8_ship, .max_progress = progress::ch9_start - 1, .story = StoryId::ch8_sendoff },
        { .type = EventType::trigger, .x = 10, .y = 1,
          .min_progress = progress::ch8_ship, .max_progress = progress::ch9_start - 1, .story = StoryId::ch8_sendoff },
        { .type = EventType::trigger, .x = 9, .y = 2,
          .min_progress = progress::ch8_ship, .max_progress = progress::ch9_start - 1, .story = StoryId::ch8_sendoff },
        { .type = EventType::trigger, .x = 17, .y = 6,
          .min_progress = progress::ch8_ship, .max_progress = progress::ch9_start - 1, .story = StoryId::ch8_sendoff },
        { .type = EventType::trigger, .x = 18, .y = 5,
          .min_progress = progress::ch8_ship, .max_progress = progress::ch9_start - 1, .story = StoryId::ch8_sendoff },
        { .type = EventType::trigger, .x = 18, .y = 7,
          .min_progress = progress::ch8_ship, .max_progress = progress::ch9_start - 1, .story = StoryId::ch8_sendoff },
        // The elder of the refugees.
        { .type = EventType::story, .x = 11, .y = 5, .npc_graphics = 1, .story = StoryId::ch7_keeper },
        // E703: the campfire (the 'A' tile). The vow happens once the four are together again.
        { .type = EventType::sign, .x = 14, .y = 2, .pages = refuge_shrine_before_pages,
          .max_progress = progress::ch7_partner - 1 },
        { .type = EventType::story, .x = 14, .y = 2,
          .min_progress = progress::ch7_partner, .max_progress = progress::ch7_vow - 1,
          .story = StoryId::ch7_campfire },
        { .type = EventType::sign, .x = 14, .y = 2, .pages = refuge_shrine_after_pages,
          .min_progress = progress::ch7_vow },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // D07: the pass east of the valley. General 1 (npc_graphics 8) comes up the last corridor.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view pass_rows[] = {
        "%%%%%%%%%%%%%%%%%%%%",    // 0
        "%u___%%%%%_______%%%",    // 1
        "%____%%%%%_%%%%%_%%%",    // 2
        "%_%%%%%%%%_%%%%%_%%%",    // 3
        "%_%%%______%%%%%_%%%",    // 4
        "%_%%%_%%%%%%%%___%%%",    // 5
        "%_____%%%%%%%%__>%%%",    // 6
        "%%%%%%%%%%%%%%%%%%%%",    // 7
    };

    static_assert(valid_layout(pass_rows), "Invalid pass layout");

    constexpr MapEvent pass_events[] = {
        { .type = EventType::warp, .x = 1, .y = 1, .warp_map = MapId::refuge, .warp_x = 18, .warp_y = 6 },
        // E702: the defector at the broken bridge (the only way through).
        { .type = EventType::trigger, .x = 16, .y = 3,
          .max_progress = progress::ch7_defector - 1, .story = StoryId::ch7_defector },
        { .type = EventType::chest, .x = 15, .y = 6, .npc_graphics = 3,
          .flag = FlagId::chest_pass, .gold = 500 },
        // On to the fort in the marsh.
        { .type = EventType::warp, .x = 16, .y = 6, .warp_map = MapId::fort_marsh, .warp_x = 9, .warp_y = 11 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // D09: the Fort of the Weeping Marsh. The partner waits at its entrance (E702); General Bardos
    // (npc_graphics 8) holds the corridor to the keep (E704). Behind him, the way down to the harbor.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view fort_marsh_rows[] = {
        "####################",    // 0
        "#########>##########",    // 1
        "#######_____########",    // 2
        "#######_____########",    // 3
        "#########_##########",    // 4
        "##______________####",    // 5
        "##_####~~~~####_####",    // 6
        "##_####~~~~####_####",    // 7
        "##______________####",    // 8
        "#########_##########",    // 9
        "~~~~~~~~_=_~~~~~~~~~",    // 10
        "~~~~~~~~~=~~~~~~~~~~",    // 11
        "~~~~~~~~~>~~~~~~~~~~",    // 12
    };

    static_assert(valid_layout(fort_marsh_rows), "Invalid marsh fort layout");

    constexpr MapEvent fort_marsh_events[] = {
        { .type = EventType::warp, .x = 9, .y = 12, .warp_map = MapId::pass, .warp_x = 16, .warp_y = 5 },
        // E702: the partner, watching the fort from the causeway.
        { .type = EventType::trigger, .x = 9, .y = 10,
          .max_progress = progress::ch7_partner - 1, .story = StoryId::ch7_partner },
        // The gate stays shut until the vow by the campfire (E703).
        { .type = EventType::trigger, .x = 9, .y = 9,
          .max_progress = progress::ch7_vow - 1, .story = StoryId::ch7_fort_gate },
        // E704.
        { .type = EventType::story, .x = 9, .y = 4, .npc_graphics = 8,
          .min_progress = progress::ch7_vow, .max_progress = progress::ch7_bardos - 1,
          .story = StoryId::ch7_bardos },
        { .type = EventType::warp, .x = 9, .y = 1, .warp_map = MapId::harbor, .warp_x = 1, .warp_y = 6 },
    };

    constexpr EncounterGroup fort_marsh_encounters[] = {
        { { EnemyId::ash_zealot, EnemyId::cult_knight }, 2 },
        { { EnemyId::cult_knight, EnemyId::cult_knight, EnemyId::cult_knight }, 3 },
        { { EnemyId::ash_zealot, EnemyId::ash_zealot }, 2 },
    };

    constexpr EncounterGroup pass_encounters[] = {
        { { EnemyId::ash_zealot, EnemyId::ash_zealot }, 2 },
        { { EnemyId::ash_zealot, EnemyId::cult_knight }, 2 },
        { { EnemyId::cult_knight, EnemyId::cult_knight, EnemyId::cult_knight }, 3 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // T08: the harbor town beyond the pass. The pier ('=') leads to the ship once it is launched.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view harbor_rows[] = {
        "xxxxxxxxxxxxxxxxxxxxxx",    // 0
        "xaaaaaaaaaaaaaaaa~~~~~",    // 1
        "xa^^^^aaaaxxaaaaa~~~~~",    // 2
        "xa####aaaaxxaaaaa~~~~~",    // 3
        "xa#DI#aaaaaaaaaaa~~~~~",    // 4
        "xaa:aaaaaaaaaaaaa~~~~~",    // 5
        ">::::::::::::::::===>~",    // 6
        "xaaaaaaa:aaaaaaaa~~~~~",    // 7
        "xaaFaaaa:aaaSaaaa~~~~~",    // 8
        "xaaaaaaa:aaaaaaaa~~~~~",    // 9
        "xaaaaaaaaaaaaaaaa~~~~~",    // 10
        "xxxxxxxx>xxxxxxxxxxxxx",    // 11
    };

    static_assert(valid_layout(harbor_rows), "Invalid harbor layout");

    constexpr bn::string_view harbor_sign_pages[] = {
        RPG_T(RPG_NAME_HARBOR " - HARBOR\nEast: the pier",
              "港町" RPG_NAME_HARBOR_JA "\n東: 桟橋"),
    };

    constexpr bn::string_view harbor_shop_pages[] = {
        RPG_T("Ship's stores. Nobody is\nsailing, so they are\nyours.",
              "船の備品だよ。誰も海に\n出ないんだ、\n持っていきな。"),
    };

    constexpr bn::string_view harbor_fisher_ch8_pages[] = {
        RPG_T("The citadel is across the\nstrait. On clear nights\nyou can see its fires.",
              "城塞は海峡の向こうだ。\n晴れた夜には、\nあそこの火が見える。"),
    };

    constexpr bn::string_view harbor_fisher_ch9_pages[] = {
        RPG_T("The citadel is dark.\nFirst dark night\nin ten years.",
              "城塞の火が消えた。\n暗い夜なんて、\n十年ぶりだ。"),
    };

    // The harbor can't be reached before chapter 8: the first eight sets are never shown.
    constexpr bn::span<const bn::string_view> harbor_fisher_chapter_pages[] = {
        harbor_fisher_ch8_pages, harbor_fisher_ch8_pages, harbor_fisher_ch8_pages, harbor_fisher_ch8_pages,
        harbor_fisher_ch8_pages, harbor_fisher_ch8_pages, harbor_fisher_ch8_pages, harbor_fisher_ch8_pages,
        harbor_fisher_ch9_pages,
    };

    constexpr MapEvent harbor_events[] = {
        { .type = EventType::warp, .x = 0, .y = 6, .warp_map = MapId::fort_marsh, .warp_x = 9, .warp_y = 2 },
        // The pier: closed without the compass (E802), then the ship (ch8_pier chooses where it sails).
        { .type = EventType::trigger, .x = 20, .y = 6,
          .max_progress = progress::ch10_start - 1, .story = StoryId::ch8_pier },
        // Chapter 10: the ship sails home instead.
        { .type = EventType::trigger, .x = 20, .y = 6,
          .min_progress = progress::ch10_start, .story = StoryId::ch10_voyage },
        // The causeway to the Sunken Lighthouse (D10).
        { .type = EventType::trigger, .x = 8, .y = 11,
          .max_progress = progress::ch8_goals - 1, .story = StoryId::ch8_lighthouse_gate },
        { .type = EventType::warp, .x = 8, .y = 11, .warp_map = MapId::lighthouse, .warp_x = 9, .warp_y = 10,
          .min_progress = progress::ch8_goals },
        { .type = EventType::inn, .x = 4, .y = 5, .npc_graphics = 2, .pages = innkeeper_pages },
        { .type = EventType::shop, .x = 11, .y = 5, .npc_graphics = 5, .pages = harbor_shop_pages,
          .stock = ashbell_shop_stock },
        { .type = EventType::sign, .x = 12, .y = 8, .pages = harbor_sign_pages },
        { .type = EventType::spring, .x = 3, .y = 8, .pages = ashbell_spring_pages, .price = 0 },
        { .type = EventType::talk, .x = 6, .y = 9, .npc_graphics = 0,
          .movement = NpcMovement::wander, .wander_radius = 2, .chapter_pages = harbor_fisher_chapter_pages },
        { .type = EventType::story, .x = 15, .y = 5, .npc_graphics = 1, .story = StoryId::ch8_shipwright },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // D10: the Sunken Lighthouse, at the end of a causeway south of the port. The thing around the lamp (the 'A'
    // tile; npc_graphics 10) holds the ship's compass.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view lighthouse_rows[] = {
        "~~~~~~~~~~~~~~~~~~~~",    // 0
        "~~~~~~~%%%%%%%~~~~~~",    // 1
        "~~~~~~~%__A__%~~~~~~",    // 2
        "~~~~~~~%_____%~~~~~~",    // 3
        "~~~~~~~%%%_%%%~~~~~~",    // 4
        "~~~%%%%%%%_%%%%%%~~~",    // 5
        "~~~%____________%~~~",    // 6
        "~~~%_%%%%~~%%%%_%~~~",    // 7
        "~~~%____________%~~~",    // 8
        "~~~%%%%%%_%%%%%%%~~~",    // 9
        "~~~~~~~~~=~~~~~~~~~~",    // 10
        "~~~~~~~~~>~~~~~~~~~~",    // 11
    };

    static_assert(valid_layout(lighthouse_rows), "Invalid lighthouse layout");

    constexpr bn::string_view lighthouse_lamp_pages[] = {
        RPG_T("The great lamp of the\nlighthouse.",
              "灯台の、大きな灯だ。"),
    };

    constexpr MapEvent lighthouse_events[] = {
        { .type = EventType::warp, .x = 9, .y = 11, .warp_map = MapId::harbor, .warp_x = 8, .warp_y = 10 },
        // E802.
        { .type = EventType::story, .x = 10, .y = 4, .npc_graphics = 10,
          .min_progress = progress::ch8_goals, .max_progress = progress::ch8_ship - 1,
          .story = StoryId::ch8_lighthouse },
        { .type = EventType::sign, .x = 10, .y = 2, .pages = lighthouse_lamp_pages },
        { .type = EventType::chest, .x = 4, .y = 7, .npc_graphics = 3,
          .flag = FlagId::chest_lighthouse, .item = ItemId::magic_water, .item_amount = 3 },
    };

    constexpr EncounterGroup lighthouse_encounters[] = {
        { { EnemyId::skeleton, EnemyId::skeleton, EnemyId::bat }, 3 },
        { { EnemyId::cult_knight, EnemyId::cult_knight }, 2 },
        { { EnemyId::ash_zealot, EnemyId::skeleton }, 2 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // D11: the Tower of the Ice Wall, in the north, reached by ship. General Ryuze (npc_graphics 8) waits in the
    // top hall until he is beaten (FlagId::ryuze_defeated).
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view ice_tower_rows[] = {
        "%%%%%%%%%%%%%%%%%%%%",    // 0
        "%%%%%%%_____%%%%%%%%",    // 1
        "%%%%%%%_____%%%%%%%%",    // 2
        "%%%%%%%%%_%%%%%%%%%%",    // 3
        "%%_______________%%%",    // 4
        "%%_%%%%%%%%%%%%%_%%%",    // 5
        "%%_______________%%%",    // 6
        "%%%%%%%%%_%%%%%%%%%%",    // 7
        "%%%%%%%%%>%%%%%%%%%%",    // 8
    };

    static_assert(valid_layout(ice_tower_rows), "Invalid ice tower layout");

    constexpr MapEvent ice_tower_events[] = {
        { .type = EventType::warp, .x = 9, .y = 8, .warp_map = MapId::harbor, .warp_x = 19, .warp_y = 6 },
        // E803.
        { .type = EventType::story, .x = 9, .y = 2, .npc_graphics = 8,
          .min_progress = progress::ch8_ship, .story = StoryId::ch8_ryuze, .gone_flag = FlagId::ryuze_defeated },
        { .type = EventType::chest, .x = 16, .y = 5, .npc_graphics = 3,
          .flag = FlagId::chest_ice_tower, .item = ItemId::herb, .item_amount = 5 },
    };

    constexpr EncounterGroup ice_tower_encounters[] = {
        { { EnemyId::ash_zealot, EnemyId::ash_zealot }, 2 },
        { { EnemyId::skeleton, EnemyId::skeleton, EnemyId::skeleton }, 3 },
        { { EnemyId::ash_zealot, EnemyId::cult_knight, EnemyId::cult_knight }, 3 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // D08: the island citadel. The three remaining generals (npc_graphics 8) hold the way to the rite circle ('A'),
    // one after another: the gate corridor, the upper corridor and the hall.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view citadel_rows[] = {
        "####################",    // 0
        "########_A_#########",    // 1
        "#######_____########",    // 2
        "#########_##########",    // 3
        "####_____________###",    // 4
        "####_###########_###",    // 5
        "####_###########_###",    // 6
        "####_____________###",    // 7
        "#########_##########",    // 8
        "#######_____########",    // 9
        "#######_____########",    // 10
        "#########>##########",    // 11
        "####################",    // 12
    };

    static_assert(valid_layout(citadel_rows), "Invalid citadel layout");

    constexpr bn::string_view citadel_circle_pages[] = {
        RPG_T("A circle of ash on the\nfloor. Whatever rite it\nwas for, it is cold.",
              "床に灰でえがかれた円。\n何の儀式のものであれ、\nもう冷えきっている。"),
    };

    constexpr MapEvent citadel_events[] = {
        { .type = EventType::warp, .x = 9, .y = 11, .warp_map = MapId::harbor, .warp_x = 19, .warp_y = 6 },
        // E804: General Drake (npc_graphics 8) in the hall, until he is beaten (FlagId::drake_defeated).
        { .type = EventType::story, .x = 9, .y = 3, .npc_graphics = 8,
          .min_progress = progress::ch8_ship, .story = StoryId::ch8_drake, .gone_flag = FlagId::drake_defeated },
        { .type = EventType::sign, .x = 9, .y = 1, .pages = citadel_circle_pages },
        { .type = EventType::chest, .x = 4, .y = 5, .npc_graphics = 3,
          .flag = FlagId::chest_citadel, .item = ItemId::magic_water, .item_amount = 5 },
    };

    constexpr EncounterGroup citadel_encounters[] = {
        { { EnemyId::ash_zealot, EnemyId::ash_zealot }, 2 },
        { { EnemyId::ash_zealot, EnemyId::cult_knight, EnemyId::cult_knight }, 3 },
        { { EnemyId::ash_zealot, EnemyId::ash_zealot, EnemyId::ash_zealot }, 3 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // D09: the Flame Emperor's castle, the last dungeon. A winding way up to the throne room; a spring ('F') and a
    // chest wait just before it. The Emperor (npc_graphics 8) stands in front of his pyre ('A').
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view castle_rows[] = {
        "####################",    // 0
        "########_Ao#########",    // 1
        "#######_____########",    // 2
        "#######_____########",    // 3
        "#########_##########",    // 4
        "####___F______######",    // 5
        "####_########_######",    // 6
        "####_########_######",    // 7
        "####_____________###",    // 8
        "################_###",    // 9
        "###______________###",    // 10
        "###_################",    // 11
        "###______________###",    // 12
        "#########_##########",    // 13
        "#########>##########",    // 14
        "####################",    // 15
    };

    static_assert(valid_layout(castle_rows), "Invalid castle layout");

    constexpr bn::string_view castle_spring_pages[] = {
        RPG_T("Clear water, in the heart\nof the fire. Someone kept\nit clean.",
              "炎のただ中に、すんだ水。\n誰かが、きれいに\n保っていたのだ。"),
    };

    constexpr bn::string_view castle_pyre_pages[] = {
        RPG_T("The Emperor's throne.\nIt is covered in ash.",
              "焔帝の玉座だ。\n灰をかぶっている。"),
    };

    constexpr MapEvent castle_events[] = {
        // E901: the ship is gone.
        { .type = EventType::trigger, .x = 9, .y = 14, .story = StoryId::ch9_no_return },
        // E902: General Zelma (npc_graphics 8) holds the only way up.
        { .type = EventType::story, .x = 16, .y = 9, .npc_graphics = 8,
          .max_progress = progress::ch9_zelma - 1, .story = StoryId::ch9_zelma },
        { .type = EventType::spring, .x = 7, .y = 5, .pages = castle_spring_pages, .price = 0 },
        { .type = EventType::chest, .x = 5, .y = 5, .npc_graphics = 3,
          .flag = FlagId::chest_castle, .item = ItemId::magic_water, .item_amount = 5 },
        // E903: the Emperor (npc_graphics 8) in his room, until he leaves for the altar.
        { .type = EventType::story, .x = 9, .y = 3, .npc_graphics = 8,
          .max_progress = progress::ch9_motive - 1, .story = StoryId::ch9_motive },
        { .type = EventType::sign, .x = 9, .y = 1, .pages = castle_pyre_pages },
        // The circle behind the throne leads to the Altar of Rebirth.
        { .type = EventType::warp, .x = 10, .y = 1, .warp_map = MapId::rebirth_altar, .warp_x = 9, .warp_y = 4 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // D14: the Altar of Rebirth. Examining the altar ('A') starts the last two battles (E904).
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view rebirth_altar_rows[] = {
        "####################",    // 0
        "########_A_#########",    // 1
        "#######_____########",    // 2
        "#######_____########",    // 3
        "#########_##########",    // 4
        "#########o##########",    // 5
        "####################",    // 6
    };

    static_assert(valid_layout(rebirth_altar_rows), "Invalid rebirth altar layout");

    constexpr bn::string_view rebirth_altar_dark_pages[] = {
        RPG_T("The altar is dark.",
              "祭壇の火は、消えている。"),
    };

    constexpr MapEvent rebirth_altar_events[] = {
        { .type = EventType::warp, .x = 9, .y = 5, .warp_map = MapId::castle, .warp_x = 10, .warp_y = 2 },
        { .type = EventType::story, .x = 9, .y = 1,
          .max_progress = progress::ch10_start - 1, .story = StoryId::ch9_altar },
        { .type = EventType::sign, .x = 9, .y = 1, .pages = rebirth_altar_dark_pages },
    };

    constexpr EncounterGroup castle_encounters[] = {
        { { EnemyId::ash_zealot, EnemyId::ash_zealot, EnemyId::ash_zealot }, 3 },
        { { EnemyId::ash_zealot, EnemyId::cult_knight, EnemyId::cult_knight }, 3 },
        { { EnemyId::cult_knight, EnemyId::cult_knight }, 2 },
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Map table. The order must match MapId.
    // ---------------------------------------------------------------------------------------------------------------
    constexpr MapData maps[] = {
        {
            .name = RPG_T(RPG_NAME_VILLAGE " Village", RPG_NAME_VILLAGE_JA "村"),
            .rows = village_rows,
            .events = village_events,
            .border = TileId::tree,
        },
        {
            .name = RPG_T("Fields", "草原"),
            .rows = field_rows,
            .events = field_events,
            .encounters = field_encounters,
            .encounter_rate = 20,
            .border = TileId::rock,
        },
        {
            .name = RPG_T("Watch Cave 1F", "見張りの洞窟 1F"),
            .rows = cave_1f_rows,
            .events = cave_1f_events,
            .encounters = cave_1f_encounters,
            .encounter_rate = 12,
            .border = TileId::cave_wall,
        },
        {
            .name = RPG_T("Watch Cave 2F", "見張りの洞窟 2F"),
            .rows = cave_2f_rows,
            .events = cave_2f_events,
            .encounters = cave_2f_encounters,
            .encounter_rate = 14,
            .border = TileId::cave_wall,
        },
        {
            .name = RPG_T("Watch Cave 3F", "見張りの洞窟 3F"),
            .rows = cave_3f_rows,
            .events = cave_3f_events,
            .encounters = cave_3f_encounters,
            .encounter_rate = 16,
            .border = TileId::cave_wall,
        },
        {
            .name = RPG_T(RPG_NAME_TOWN, RPG_NAME_TOWN_JA),
            .rows = berg_rows,
            .events = berg_events,
            .border = TileId::tree,
        },
        {
            .name = RPG_T("Ruined Fort", "廃砦"),
            .rows = fort_rows,
            .events = fort_events,
            .encounters = fort_encounters,
            .encounter_rate = 12,
            .border = TileId::wall,
        },
        {
            .name = RPG_T(RPG_NAME_SEER_TOWN, RPG_NAME_SEER_TOWN_JA),
            .rows = lunaris_rows,
            .events = lunaris_events,
            .border = TileId::tree,
        },
        {
            .name = RPG_T("Star Ruins", "星見の遺跡"),
            .rows = ruins_rows,
            .events = ruins_events,
            .encounters = ruins_encounters,
            .encounter_rate = 12,
            .border = TileId::cave_wall,
        },
        {
            .name = RPG_T("Old Waterway", "古い水路"),
            .rows = waterway_rows,
            .events = waterway_events,
            .encounters = waterway_encounters,
            .encounter_rate = 12,
            .border = TileId::cave_wall,
        },
        {
            .name = RPG_T("Scorched Plains", "焦土の平原"),
            .rows = wasteland_rows,
            .events = wasteland_events,
            .encounters = wasteland_encounters,
            .encounter_rate = 10,
            .border = TileId::cave_wall,
        },
        {
            .name = RPG_T(RPG_NAME_ASH_TOWN, RPG_NAME_ASH_TOWN_JA),
            .rows = ashbell_rows,
            .events = ashbell_events,
            .border = TileId::rubble,
        },
        {
            .name = RPG_T("Searing Canyon", "焦熱の峡谷"),
            .rows = canyon_rows,
            .events = canyon_events,
            .encounters = canyon_encounters,
            .encounter_rate = 12,
            .border = TileId::cave_wall,
        },
        {
            .name = RPG_T(RPG_NAME_MINE_TOWN, RPG_NAME_MINE_TOWN_JA),
            .rows = dorn_rows,
            .events = dorn_events,
            .border = TileId::cave_wall,
        },
        {
            .name = RPG_T(RPG_NAME_PARTNER_HOME " Ruins", RPG_NAME_PARTNER_HOME_JA "村跡"),
            .rows = kazami_rows,
            .events = kazami_events,
            .border = TileId::rubble,
        },
        {
            .name = RPG_T("Ash Cathedral", "灰の聖堂"),
            .rows = cathedral_rows,
            .events = cathedral_events,
            .encounters = cathedral_encounters,
            .encounter_rate = 12,
            .border = TileId::wall,
        },
        {
            .name = RPG_T(RPG_NAME_CAPITAL, RPG_NAME_CAPITAL_JA),
            .rows = grancel_rows,
            .events = grancel_events,
            .border = TileId::wall,
        },
        {
            .name = RPG_T(RPG_NAME_REFUGE, RPG_NAME_REFUGE_JA),
            .rows = refuge_rows,
            .events = refuge_events,
            .border = TileId::tree,
        },
        {
            .name = RPG_T("Central Highway", "中央街道"),
            .rows = pass_rows,
            .events = pass_events,
            .encounters = pass_encounters,
            .encounter_rate = 12,
            .border = TileId::cave_wall,
        },
        {
            .name = RPG_T(RPG_NAME_HARBOR, RPG_NAME_HARBOR_JA),
            .rows = harbor_rows,
            .events = harbor_events,
            .border = TileId::water,
        },
        {
            .name = RPG_T("Volcano Fort", "火山の砦"),
            .rows = citadel_rows,
            .events = citadel_events,
            .encounters = citadel_encounters,
            .encounter_rate = 12,
            .border = TileId::wall,
        },
        {
            .name = RPG_T("Emperor's Castle", "焔帝の城"),
            .rows = castle_rows,
            .events = castle_events,
            .encounters = castle_encounters,
            .encounter_rate = 10,
            .border = TileId::wall,
        },
        {
            .name = RPG_T("Abandoned Mine", RPG_NAME_MINE_TOWN_JA "廃坑"),
            .rows = mine_rows,
            .events = mine_events,
            .encounters = mine_encounters,
            .encounter_rate = 12,
            .border = TileId::cave_wall,
        },
        {
            .name = RPG_T("Royal Castle", "王城" RPG_NAME_CAPITAL_JA),
            .rows = throne_rows,
            .events = throne_events,
            .border = TileId::wall,
        },
        {
            .name = RPG_T("Weeping Marsh Fort", "嘆きの沼の砦"),
            .rows = fort_marsh_rows,
            .events = fort_marsh_events,
            .encounters = fort_marsh_encounters,
            .encounter_rate = 12,
            .border = TileId::water,
        },
        {
            .name = RPG_T("Sunken Lighthouse", "沈んだ灯台"),
            .rows = lighthouse_rows,
            .events = lighthouse_events,
            .encounters = lighthouse_encounters,
            .encounter_rate = 12,
            .border = TileId::water,
        },
        {
            .name = RPG_T("Ice Wall Tower", "氷壁の塔"),
            .rows = ice_tower_rows,
            .events = ice_tower_events,
            .encounters = ice_tower_encounters,
            .encounter_rate = 12,
            .border = TileId::cave_wall,
        },
        {
            .name = RPG_T("Altar of Rebirth", "再誕の祭壇"),
            .rows = rebirth_altar_rows,
            .events = rebirth_altar_events,
            .border = TileId::wall,
        },
    };

    static_assert(sizeof(maps) / sizeof(maps[0]) == int(MapId::count), "Map table doesn't match MapId");

    // ---------------------------------------------------------------------------------------------------------------
    // Map states: a different layout of a map while the story progress is in a range. Outside of every range, the
    // map of the table above is used (for the home village, that is only the peaceful morning of chapter 1).
    // ---------------------------------------------------------------------------------------------------------------
    struct MapState
    {
        MapId id;
        uint8_t min_progress;
        uint8_t max_progress;
        MapData data;
    };

    constexpr MapState map_states[] = {
        {
            MapId::grancel, progress::ch6_collapse, 0xFF,
            {
                .name = RPG_T(RPG_NAME_CAPITAL, RPG_NAME_CAPITAL_JA),
                .rows = grancel_burning_rows,
                .events = grancel_events,
                .encounters = grancel_encounters,
                .encounter_rate = 10,
                .border = TileId::rubble,
            },
        },
        {
            MapId::village, progress::ch10_rebuilt, 0xFF,
            {
                .name = RPG_T(RPG_NAME_VILLAGE " Village", RPG_NAME_VILLAGE_JA "村"),
                .rows = village_rebuilt_rows,
                .events = village_events,
                .border = TileId::tree,
                .bgm = BgmId::event_hope,
            },
        },
        {
            MapId::village, progress::ch1_ashes, progress::ch10_rebuilt - 1,
            {
                .name = RPG_T(RPG_NAME_VILLAGE " Ruins", RPG_NAME_VILLAGE_JA "村の焼け跡"),
                .rows = village_burnt_rows,
                .events = village_events,
                .border = TileId::tree,
                .bgm = BgmId::event_dark,
            },
        },
    };

    // Where the party wakes up after a defeat, by chapter_index(). Later chapters use the last one.
    constexpr RespawnPoint respawn_points[] = {
        { start_map, start_x, start_y },    // Chapter 1: home.
        { MapId::berg, 4, 6 },              // Chapter 2: in front of the inn of the post town.
        { MapId::lunaris, 4, 6 },           // Chapter 3: in front of the inn of the seer's village.
        { MapId::ashbell, 3, 6 },           // Chapter 4: in front of the inn of the ash town.
        { MapId::dorn, 4, 6 },              // Chapter 5: in front of the inn of the mining town.
        { MapId::dorn, 4, 6 },              // Chapter 6: the same (the capital has no safe place).
        { MapId::refuge, 3, 6 },            // Chapter 7: in front of the inn of the refugee valley.
        { MapId::harbor, 3, 6 },            // Chapter 8 on: in front of the inn of the harbor town.
        { MapId::castle, 9, 13 },           // Chapter 9: the shore of the island (there is no way back).
        { start_map, start_x, start_y },    // Chapter 10 and after the ending: home.
    };

    [[nodiscard]] constexpr bool walkable_tile(bn::span<const bn::string_view> rows, int x, int y)
    {
        if(x < 0 || y < 0 || y >= rows.size() || x >= rows[0].size())
        {
            return false;
        }

        return tiles[tile_index_from_symbol(rows[y][x])].walkable;
    }

    /// Compile time check of the events: a typo in a coordinate becomes a build error instead of a stuck player.
    [[nodiscard]] constexpr bool walkable_in_every_state(MapId id, int x, int y)
    {
        if(! walkable_tile(maps[int(id)].rows, x, y))
        {
            return false;
        }

        for(const MapState& state : map_states)
        {
            if(state.id == id && ! walkable_tile(state.data.rows, x, y))
            {
                return false;
            }
        }

        return true;
    }

    /// Checks the events of one layout that can exist while the story progress is in [min_progress, max_progress].
    [[nodiscard]] constexpr bool valid_events(const MapData& map, int min_progress, int max_progress)
    {
        {
            for(const MapEvent& event : map.events)
            {
                if(event.min_progress > event.max_progress)
                {
                    return false;
                }

                if(event.max_progress < min_progress || event.min_progress > max_progress)
                {
                    continue;       // Never seen in this layout.
                }

                // Scenes need a script, and only scenes have one.
                bool scripted = event.type == EventType::story || event.type == EventType::trigger;

                if(scripted != (event.story != StoryId::none))
                {
                    return false;
                }

                if(event.type == EventType::trigger && ! walkable_tile(map.rows, event.x, event.y))
                {
                    return false;
                }

                // NPCs and chests stand on walkable tiles (signs and springs are on solid ones, on purpose).
                if(event.npc_graphics >= 0 && ! walkable_tile(map.rows, event.x, event.y))
                {
                    return false;
                }

                // Only NPCs walk: they need a sprite, and chests stay where they are.
                if(event.movement != NpcMovement::stand && (event.npc_graphics < 0 || event.type == EventType::chest))
                {
                    return false;
                }

                if(event.type == EventType::warp)
                {
                    // You must be able to step on it, and to stand where it takes you.
                    if(! walkable_tile(map.rows, event.x, event.y) ||
                       ! walkable_in_every_state(event.warp_map, event.warp_x, event.warp_y))
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    [[nodiscard]] constexpr bool valid_events()
    {
        for(const MapData& map : maps)
        {
            if(! valid_events(map, 0, 0xFF))
            {
                return false;
            }
        }

        for(const MapState& state : map_states)
        {
            // All the states of a map are interchangeable: same size.
            if(state.data.width() != maps[int(state.id)].width() || state.data.height() != maps[int(state.id)].height() ||
               ! valid_events(state.data, state.min_progress, state.max_progress))
            {
                return false;
            }
        }

        for(const RespawnPoint& point : respawn_points)
        {
            if(! walkable_in_every_state(point.map, point.x, point.y))
            {
                return false;
            }
        }

        return true;
    }

    static_assert(valid_events(), "An event is on a solid tile, a warp leads to a solid tile or a scene has no script");
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

bool MapData::terrain_walkable(int x, int y) const
{
    return x >= 0 && y >= 0 && x < width() && y < height() && tile_info(tile(x, y)).walkable;
}

bool MapData::walkable(int x, int y, int story_progress, const EventFlags& flags) const
{
    if(! terrain_walkable(x, y))
    {
        return false;
    }

    // Events with graphics (NPCs, chests, shopkeepers) are solid, except the ones that walk: they may have left
    // their start tile, and FieldScene moves them away if the player is on it when the map is loaded.
    for(const MapEvent& event : events)
    {
        if(event.x == x && event.y == y && event.npc_graphics >= 0 && event.movement == NpcMovement::stand &&
           event.exists(story_progress, flags))
        {
            return false;
        }
    }

    return true;
}

const MapData& map_data(MapId id, int story_progress)
{
    BN_ASSERT(id < MapId::count, "Invalid map id: ", int(id));

    for(const MapState& state : map_states)
    {
        if(state.id == id && story_progress >= state.min_progress && story_progress <= state.max_progress)
        {
            return state.data;
        }
    }

    return maps[int(id)];
}

RespawnPoint respawn_point(int story_progress)
{
    int count = sizeof(respawn_points) / sizeof(respawn_points[0]);
    int chapter = chapter_index(story_progress);
    return respawn_points[chapter < count ? chapter : count - 1];
}

}
