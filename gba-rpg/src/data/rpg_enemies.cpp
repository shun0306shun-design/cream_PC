#include "rpg_enemies.h"

#include "bn_assert.h"

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
        { "Slime",   &bn::sprite_items::enemy_slime,      { 12, 0, 10, 4,  4  }, 4,  3,    SpellId::fire, 0 },
        { "Bat",     &bn::sprite_items::enemy_bat,        { 16, 0, 13, 5,  12 }, 6,  5,    SpellId::fire, 20 },
        { "Goblin",  &bn::sprite_items::enemy_goblin,     { 28, 0, 18, 8,  6  }, 12, 10,   SpellId::fire, 0 },
        { "Skeleton", &bn::sprite_items::enemy_skeleton,    { 34, 0, 22, 10, 8  }, 18, 16,   SpellId::fire, 0 },
        { "Cultist", &bn::sprite_items::enemy_cultist,    { 40, 0, 24, 9,  9  }, 22, 20,   SpellId::fire, 15 },
        // Story bosses (see rpg_story.cpp). The scout is fought alone, the general by two.
        { "Cult Scout", &bn::sprite_items::enemy_cultist, { 70, 0, 26, 8,  9  }, 45, 40,   SpellId::fire, 25 },
        { RPG_NAME_GENERAL_1, &bn::sprite_items::enemy_drake, { 170, 0, 36, 12, 12 }, 120, 150, SpellId::blizzard, 30 },
        // Chapter 3 bosses, fought by three.
        { "Star Guardian", &bn::sprite_items::enemy_guardian, { 230, 0, 36, 16, 7 }, 160, 120, SpellId::blizzard, 25 },
        { "Gate Warden", &bn::sprite_items::enemy_cultist, { 200, 0, 40, 13, 11 }, 200, 180, SpellId::fire, 30 },
        // Chapter 4: beyond the gate.
        { "Cult Knight", &bn::sprite_items::enemy_cultist, { 75, 0, 34, 14, 10 }, 40, 30, SpellId::fire, 20 },
        { "Ember Wyrm", &bn::sprite_items::enemy_wyrm, { 320, 0, 44, 16, 12 }, 300, 250, SpellId::fire, 35 },
    };

    static_assert(sizeof(enemies) / sizeof(enemies[0]) == int(EnemyId::count), "Enemy table doesn't match EnemyId");
}

const EnemyData& enemy_data(EnemyId id)
{
    BN_ASSERT(id < EnemyId::count, "Invalid enemy id: ", int(id));

    return enemies[int(id)];
}

}
