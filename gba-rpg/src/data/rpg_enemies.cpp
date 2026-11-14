#include "rpg_enemies.h"

#include "bn_assert.h"

#include "rpg_text.h"

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
    // The order must match EnemyId.
    constexpr EnemyData enemies[] = {
        //  name       sprite                               HP  MP ATK DEF AGI   EXP GOLD  spell          chance%
        { RPG_T("Slime", "スライム"),   &bn::sprite_items::enemy_slime,      { 12, 0, 10, 4,  4  }, 4,  3,    SpellId::fire, 0 },
        { RPG_T("Bat", "コウモリ"),     &bn::sprite_items::enemy_bat,        { 16, 0, 13, 5,  12 }, 6,  5,    SpellId::fire, 20 },
        { RPG_T("Goblin", "ゴブリン"),  &bn::sprite_items::enemy_goblin,     { 28, 0, 18, 8,  6  }, 12, 10,   SpellId::fire, 0 },
        { RPG_T("Skeleton", "スケルトン"), &bn::sprite_items::enemy_skeleton,    { 34, 0, 22, 10, 8  }, 18, 16,   SpellId::fire, 0 },
        { RPG_T("Cultist", "教団員"), &bn::sprite_items::enemy_cultist,    { 40, 0, 24, 9,  9  }, 22, 20,   SpellId::fire, 15 },
        // Story bosses (see rpg_story.cpp). The scout is fought alone, the general by two.
        { RPG_T("Cult Scout", "教団の斥候"), &bn::sprite_items::enemy_cultist, { 70, 0, 26, 8,  9  }, 45, 40,   SpellId::fire, 25 },
        { RPG_T(RPG_NAME_GENERAL_1, RPG_NAME_GENERAL_1_JA), &bn::sprite_items::enemy_drake, { 170, 0, 36, 12, 12 }, 120, 150, SpellId::blizzard, 30 },
        // Chapter 3 bosses, fought by three.
        { RPG_T("Star Guardian", "星の守護者"), &bn::sprite_items::enemy_guardian, { 230, 0, 36, 16, 7 }, 160, 120, SpellId::blizzard, 25 },
        { RPG_T("Gate Warden", "関所の番人"), &bn::sprite_items::enemy_cultist, { 200, 0, 40, 13, 11 }, 200, 180, SpellId::fire, 30 },
        // Chapter 4: beyond the gate.
        { RPG_T("Cult Knight", "教団の騎士"), &bn::sprite_items::enemy_cultist, { 75, 0, 34, 14, 10 }, 40, 30, SpellId::fire, 20 },
        { RPG_T("Ember Wyrm", "炎のワーム"), &bn::sprite_items::enemy_wyrm, { 320, 0, 44, 16, 12 }, 300, 250, SpellId::fire, 35 },
        // Chapter 5: the Ash Cathedral. The general is fought by four this time, and flees again.
        { RPG_T("Ash Zealot", "灰の狂信者"), &bn::sprite_items::enemy_cultist, { 95, 0, 40, 15, 11 }, 55, 45, SpellId::fire, 30 },
        { RPG_T(RPG_NAME_GENERAL_1, RPG_NAME_GENERAL_1_JA), &bn::sprite_items::enemy_drake, { 520, 0, 54, 20, 14 }, 500, 400, SpellId::blizzard, 35 },
        // Chapter 6: a battle the story expects to be lost (see story_defeat_continues()).
        { RPG_T(RPG_NAME_GENERAL_2, RPG_NAME_GENERAL_2_JA), &bn::sprite_items::enemy_drake, { 6000, 0, 140, 70, 40 }, 0, 0, SpellId::blizzard, 40 },
        // Chapter 7: General 1 for the last time.
        { RPG_T(RPG_NAME_GENERAL_1, RPG_NAME_GENERAL_1_JA), &bn::sprite_items::enemy_drake, { 700, 0, 60, 22, 15 }, 800, 600, SpellId::blizzard, 35 },
        // Chapter 8: the three remaining generals, in the citadel. General 2 can be beaten this time.
        { RPG_T(RPG_NAME_GENERAL_3, RPG_NAME_GENERAL_3_JA), &bn::sprite_items::enemy_drake, { 800, 0, 64, 26, 10 }, 900, 700, SpellId::blizzard, 25 },
        { RPG_T(RPG_NAME_GENERAL_4, RPG_NAME_GENERAL_4_JA), &bn::sprite_items::enemy_drake, { 650, 0, 58, 18, 24 }, 900, 700, SpellId::fire, 45 },
        { RPG_T(RPG_NAME_GENERAL_2, RPG_NAME_GENERAL_2_JA), &bn::sprite_items::enemy_drake, { 1000, 0, 70, 26, 18 }, 1500, 1000, SpellId::blizzard, 40 },
        // Chapter 9: the Flame Emperor, two battles in a row (the party is restored in between).
        { RPG_T(RPG_NAME_EMPEROR, RPG_NAME_EMPEROR_JA), &bn::sprite_items::enemy_drake, { 1400, 0, 76, 28, 20 }, 0, 0, SpellId::blizzard, 40 },
        { RPG_T("Pyre of Return", "帰還の火葬台"), &bn::sprite_items::enemy_wyrm, { 2000, 0, 88, 30, 24 }, 0, 0, SpellId::fire, 50 },
    };

    static_assert(sizeof(enemies) / sizeof(enemies[0]) == int(EnemyId::count), "Enemy table doesn't match EnemyId");
}

const EnemyData& enemy_data(EnemyId id)
{
    BN_ASSERT(id < EnemyId::count, "Invalid enemy id: ", int(id));

    return enemies[int(id)];
}

}
