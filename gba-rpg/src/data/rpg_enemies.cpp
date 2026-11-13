#include "rpg_enemies.h"

#include "bn_assert.h"

#include "bn_sprite_items_enemy_slime.h"
#include "bn_sprite_items_enemy_bat.h"
#include "bn_sprite_items_enemy_goblin.h"

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
    };

    static_assert(sizeof(enemies) / sizeof(enemies[0]) == int(EnemyId::count), "Enemy table doesn't match EnemyId");
}

const EnemyData& enemy_data(EnemyId id)
{
    BN_ASSERT(id < EnemyId::count, "Invalid enemy id: ", int(id));

    return enemies[int(id)];
}

}
