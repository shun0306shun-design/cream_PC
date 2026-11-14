/*
 * Shared constants and small value types used by every module.
 */

#ifndef RPG_COMMON_H
#define RPG_COMMON_H

#include "bn_common.h"

namespace bn
{
    class random;
}

namespace rpg
{

// --- Screen / grid -------------------------------------------------------------------------------------------------
constexpr int screen_width = 240;
constexpr int screen_height = 160;
constexpr int tile_size = 16;           //!< Size in pixels of a map tile (2x2 hardware tiles).

// --- Game rules ----------------------------------------------------------------------------------------------------
constexpr int max_party_size = 4;
constexpr int max_enemies = 3;          //!< Enemies per battle.
constexpr int max_level = 100;
constexpr int max_item_stack = 99;

/**
 * Combat statistics. Party members derive them from class + level (see rpg_classes.h),
 * enemies store them directly (see rpg_enemies.h).
 */
struct Stats
{
    int16_t max_hp = 0;
    int16_t max_mp = 0;
    int16_t attack = 0;
    int16_t defense = 0;
    int16_t agility = 0;
};

enum class Direction : uint8_t
{
    down,
    up,
    left,
    right
};

// --- Effects: shared by spells and items ---------------------------------------------------------------------------

enum class EffectType : uint8_t
{
    damage,     //!< Fixed (magic) damage to the opposing side: power + [0, variance].
    heal_hp,    //!< Restores HP of the user's side: power + [0, variance].
    heal_mp,    //!< Restores MP of the user's side: power + [0, variance].
    physical,   //!< Weapon attack: `power` percent of a normal attack, `hits` times. `chance` percent to stun.
    charge,     //!< Doubles the damage of the user's next weapon attack (battle only).
    revive      //!< Revives a fallen ally with `power` percent of its HP. Succeeds `chance` percent of the time.
};

/// Targets are relative to the user: an enemy casting a `one_enemy` spell hits a party member.
enum class TargetType : uint8_t
{
    one_enemy,
    all_enemies,
    random_enemy,   //!< Every hit picks a random enemy.
    one_ally,
    all_allies,
    self
};

struct Effect
{
    EffectType type = EffectType::damage;
    TargetType target = TargetType::one_enemy;
    int16_t power = 0;      //!< Base amount (a percentage for physical and revive effects).
    int16_t variance = 0;   //!< The final amount is power + [0, variance].
    uint8_t hits = 1;       //!< Number of hits of a physical effect.
    uint8_t chance = 0;     //!< Physical: stun chance in percent. Revive: success chance in percent.

    /// Restores something to the user's side (the only effects that can be used outside of battle).
    [[nodiscard]] constexpr bool is_heal() const
    {
        return type == EffectType::heal_hp || type == EffectType::heal_mp || type == EffectType::revive;
    }

    [[nodiscard]] constexpr bool targets_all() const
    {
        return target == TargetType::all_enemies || target == TargetType::all_allies;
    }

    /// The player has to pick who receives the effect.
    [[nodiscard]] constexpr bool needs_target_choice() const
    {
        return target == TargetType::one_enemy || target == TargetType::one_ally;
    }
};

/// Rolls the amount of an effect (damage dealt or points restored).
[[nodiscard]] int roll_effect(const Effect& effect, bn::random& random);

/// Classic physical damage formula: attack / 2 - defense / 4, with a bit of variance. Never less than 1.
[[nodiscard]] int roll_physical_damage(int attack, int defense, bn::random& random);

}

#endif
