#include "rpg_enemies.h"

#include "bn_assert.h"

#include "rpg_text.h"
#include "rpg_classes.h"

#include "bn_sprite_items_enemy_slime.h"
#include "bn_sprite_items_enemy_bat.h"
#include "bn_sprite_items_enemy_goblin.h"
#include "bn_sprite_items_enemy_skeleton.h"
#include "bn_sprite_items_enemy_cultist.h"
#include "bn_sprite_items_enemy_drake.h"
#include "bn_sprite_items_enemy_guardian.h"
#include "bn_sprite_items_enemy_wyrm.h"

#include "rpg_names.h"

namespace rpg
{

namespace
{
    using R = EnemyRank;

    // Experience: enemy_exp_for_level(level of the party that meets the enemy, percent), see rpg_classes.h.
    // The story has about 150 random battles and 12 bosses, far less than the ~670 battles that 100% assumes,
    // so its enemies give several times more. These percentages are tuned for this pace (hero's level):
    //   ch.1 boss: 11   ch.2: 15   ch.3: 20   ch.4: 28   ch.5: 36   ch.7: 46   ch.8: 56   final boss: ~60
    // A party that runs from most fights arrives around Lv 52, one that fights everything around Lv 66.
    constexpr int early_percent = 400;      // Chapters 1-3.

    // The order must match EnemyId.
    constexpr EnemyData enemies[] = {
        //  name, sprite
        //      HP    MP ATK  DEF AGI    rank          EXP                              GOLD  spell              chance%
        { RPG_T("Slime", "スライム"), &bn::sprite_items::enemy_slime,
              { 12,   0, 10,  4,  4  }, R::normal,    enemy_exp_for_level(2, early_percent),  3,    SpellId::fire,     0 },
        { RPG_T("Bat", "コウモリ"), &bn::sprite_items::enemy_bat,
              { 16,   0, 13,  5,  12 }, R::normal,    enemy_exp_for_level(3, early_percent),  5,    SpellId::fire,     20 },
        { RPG_T("Goblin", "ゴブリン"), &bn::sprite_items::enemy_goblin,
              { 28,   0, 18,  8,  6  }, R::normal,    enemy_exp_for_level(5, early_percent),  10,   SpellId::fire,     0 },
        { RPG_T("Skeleton", "スケルトン"), &bn::sprite_items::enemy_skeleton,
              { 34,   0, 22,  10, 8  }, R::normal,    enemy_exp_for_level(9, early_percent),  16,   SpellId::fire,     0 },
        { RPG_T("Cultist", "教団員"), &bn::sprite_items::enemy_cultist,
              { 40,   0, 24,  9,  9  }, R::normal,    enemy_exp_for_level(13, early_percent), 20,   SpellId::fire,     15 },
        // Story bosses (see rpg_story.cpp). The scout is fought alone, the general by two.
        { RPG_T("Cult Scout", "教団の斥候"), &bn::sprite_items::enemy_cultist,
              { 70,   0, 26,  8,  9  }, R::boss,      enemy_exp_for_level(10, 1000),          40,   SpellId::fire,     25 },
        { RPG_T(RPG_NAME_GENERAL_1, RPG_NAME_GENERAL_1_JA), &bn::sprite_items::enemy_drake,
              { 170,  0, 36,  12, 12 }, R::boss,      enemy_exp_for_level(14, 1000),          150,  SpellId::blizzard, 30 },
        // Chapter 3 bosses, fought by three.
        { RPG_T("Star Guardian", "星の守護者"), &bn::sprite_items::enemy_guardian,
              { 230,  0, 36,  16, 7  }, R::boss,      enemy_exp_for_level(17, 1000),          120,  SpellId::blizzard, 25 },
        { RPG_T("Gate Warden", "関所の番人"), &bn::sprite_items::enemy_cultist,
              { 200,  0, 40,  13, 11 }, R::boss,      enemy_exp_for_level(20, 1000),          180,  SpellId::fire,     30 },
        // Chapter 4: beyond the gate.
        { RPG_T("Cult Knight", "教団の騎士"), &bn::sprite_items::enemy_cultist,
              { 75,   0, 34,  14, 10 }, R::normal,    enemy_exp_for_level(28, 500),           30,   SpellId::fire,     20 },
        { RPG_T("Ember Wyrm", "炎のワーム"), &bn::sprite_items::enemy_wyrm,
              { 320,  0, 44,  16, 12 }, R::boss,      enemy_exp_for_level(27, 2000),          250,  SpellId::fire,     35 },
        // Chapter 5: the abandoned mine and the Ash Cathedral. The general (Zelma) flees when she is beaten.
        // The zealots are met until the end of the game: that's why they give so much.
        { RPG_T("Ash Zealot", "灰の狂信者"), &bn::sprite_items::enemy_cultist,
              { 95,   0, 40,  15, 11 }, R::normal,    enemy_exp_for_level(45, 800),           45,   SpellId::fire,     30 },
        { RPG_T(RPG_NAME_GENERAL_3, RPG_NAME_GENERAL_3_JA), &bn::sprite_items::enemy_drake,
              { 520,  0, 54,  20, 14 }, R::boss,      enemy_exp_for_level(35, 3000),          400,  SpellId::blizzard, 35 },
        { RPG_T("Mine Overseer", "廃坑の監督官"), &bn::sprite_items::enemy_cultist,
              { 360,  0, 48,  18, 10 }, R::boss,      enemy_exp_for_level(33, 2000),          300,  SpellId::fire,     25 },
        // Chapter 6: a battle the story expects to be lost (see story_defeat_continues()).
        { RPG_T(RPG_NAME_GENERAL_2, RPG_NAME_GENERAL_2_JA), &bn::sprite_items::enemy_drake,
              { 6000, 0, 140, 70, 40 }, R::boss,      0,                                      0,    SpellId::blizzard, 40 },
        // Chapter 7: General 1 for the last time.
        { RPG_T(RPG_NAME_GENERAL_2, RPG_NAME_GENERAL_2_JA), &bn::sprite_items::enemy_drake,
              { 700,  0, 60,  22, 15 }, R::boss,      enemy_exp_for_level(45, 5000),          600,  SpellId::blizzard, 35 },
        // Chapter 8: the three remaining generals, in the citadel. General 2 can be beaten this time.
        { RPG_T(RPG_NAME_GENERAL_3, RPG_NAME_GENERAL_3_JA), &bn::sprite_items::enemy_drake,
              { 800,  0, 64,  26, 10 }, R::boss,      enemy_exp_for_level(52, 5000),          700,  SpellId::blizzard, 25 },
        { RPG_T(RPG_NAME_GENERAL_4, RPG_NAME_GENERAL_4_JA), &bn::sprite_items::enemy_drake,
              { 650,  0, 58,  18, 24 }, R::boss,      enemy_exp_for_level(54, 5000),          700,  SpellId::fire,     45 },
        { RPG_T(RPG_NAME_GENERAL_1, RPG_NAME_GENERAL_1_JA), &bn::sprite_items::enemy_drake,
              { 1000, 0, 70,  26, 18 }, R::boss,      enemy_exp_for_level(56, 6000),          1000, SpellId::blizzard, 40 },
        // Chapter 9: the Flame Emperor, two battles in a row (the party is restored in between).
        { RPG_T(RPG_NAME_EMPEROR, RPG_NAME_EMPEROR_JA), &bn::sprite_items::enemy_drake,
              { 1400, 0, 76,  28, 20 }, R::last_boss, 0,                                      0,    SpellId::blizzard, 40 },
        { RPG_T("Rebirth Flame", "再誕の炎"), &bn::sprite_items::enemy_wyrm,
              { 2000, 0, 88,  30, 24 }, R::last_boss, 0,                                      0,    SpellId::fire,     50 },
        // Chapter 8: the Sunken Lighthouse.
        { RPG_T("Drowned Keeper", "沈んだ灯台守"), &bn::sprite_items::enemy_wyrm,
              { 600,  0, 58,  20, 12 }, R::boss,      enemy_exp_for_level(40, 3000),          500,  SpellId::blizzard, 30 },
    };

    static_assert(sizeof(enemies) / sizeof(enemies[0]) == int(EnemyId::count), "Enemy table doesn't match EnemyId");

    // Enemy actions per round (standard difficulty). The order must match EnemyRank.
    constexpr EnemyActionChances action_chances[] = {
        //  x1   x2  x3
        { { 100, 0,  0  } },    // normal
        { { 70,  30, 0  } },    // boss
        { { 60,  30, 10 } },    // last_boss
    };

    static_assert(sizeof(action_chances) / sizeof(action_chances[0]) == int(EnemyRank::count),
                  "Action chances table doesn't match EnemyRank");

    [[nodiscard]] constexpr bool valid_action_chances()
    {
        for(const EnemyActionChances& chances : action_chances)
        {
            int total = 0;

            for(uint8_t percent : chances.percent)
            {
                total += percent;
            }

            if(total != 100)
            {
                return false;
            }
        }

        return true;
    }

    static_assert(valid_action_chances(), "Enemy action chances must add up to 100");
}

const EnemyActionChances& enemy_action_chances(EnemyRank rank)
{
    BN_ASSERT(rank < EnemyRank::count, "Invalid enemy rank: ", int(rank));

    return action_chances[int(rank)];
}

const EnemyData& enemy_data(EnemyId id)
{
    BN_ASSERT(id < EnemyId::count, "Invalid enemy id: ", int(id));

    return enemies[int(id)];
}

}
