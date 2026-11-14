#include "rpg_enemies.h"

#include "bn_assert.h"

#include "rpg_classes.h"

#include "bn_sprite_items_enemy_slime.h"
#include "bn_sprite_items_enemy_bat.h"
#include "bn_sprite_items_enemy_goblin.h"

namespace rpg
{

namespace
{
    using R = EnemyRank;

    // Experience: enemy_exp_for_level(area level, percent) keeps the pace described in rpg_classes.h
    // (main story cleared around Lv 60). Bosses give about 10 times more than the enemies around them.
    constexpr int boss_exp_percent = 1000;

    // The order must match EnemyId.
    constexpr EnemyData enemies[] = {
        //  name            sprite                           HP    MP ATK  DEF  AGI    rank
        //      EXP                                              GOLD   spell               chance%
        { "Slime",        &bn::sprite_items::enemy_slime,  { 12,   0, 10,  4,   4   }, R::normal,
                enemy_exp_for_level(1, 150),                     3,     SpellId::mera,      0 },
        { "Bat",          &bn::sprite_items::enemy_bat,    { 16,   0, 13,  5,   12  }, R::normal,
                enemy_exp_for_level(2),                          5,     SpellId::mera,      20 },
        { "Goblin",       &bn::sprite_items::enemy_goblin, { 28,   0, 18,  8,   6   }, R::normal,
                enemy_exp_for_level(3),                          10,    SpellId::mera,      0 },

        // Samples of the stronger ranks (placeholder sprites). They aren't in any encounter group yet:
        // add them to a map's EncounterGroup or to a future boss event.
        // Goblin King is meant for a Lv 12 party and Dark Lord for a Lv 60 one.
        { "Goblin King",  &bn::sprite_items::enemy_goblin, { 450,  0, 70,  24,  14  }, R::boss,
                enemy_exp_for_level(12, boss_exp_percent),       300,   SpellId::mera,      25 },
        { "Dark Lord",    &bn::sprite_items::enemy_goblin, { 6000, 0, 330, 140, 110 }, R::last_boss,
                enemy_exp_for_level(60, boss_exp_percent),       0,     SpellId::merazoma,  25 },
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
