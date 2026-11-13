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
constexpr int max_level = 50;
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
    damage,     //!< Reduces HP of the opposing side.
    heal_hp,    //!< Restores HP of the user's side.
    heal_mp     //!< Restores MP of the user's side.
};

/// Targets are relative to the user: an enemy casting a `one_enemy` spell hits a party member.
enum class TargetType : uint8_t
{
    one_enemy,
    all_enemies,
    one_ally,
    all_allies
};

struct Effect
{
    EffectType type = EffectType::damage;
    TargetType target = TargetType::one_enemy;
    int16_t power = 0;      //!< Base amount.
    int16_t variance = 0;   //!< The final amount is power + [0, variance].

    [[nodiscard]] constexpr bool is_heal() const
    {
        return type != EffectType::damage;
    }

    [[nodiscard]] constexpr bool targets_all() const
    {
        return target == TargetType::all_enemies || target == TargetType::all_allies;
    }
};

/// Rolls the amount of an effect (damage dealt or points restored).
[[nodiscard]] int roll_effect(const Effect& effect, bn::random& random);

/// Classic physical damage formula: attack / 2 - defense / 4, with a bit of variance. Never less than 1.
[[nodiscard]] int roll_physical_damage(int attack, int defense, bn::random& random);

}

#endif
