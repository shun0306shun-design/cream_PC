#include "rpg_spells.h"

#include "bn_assert.h"

namespace rpg
{

namespace
{
    using E = EffectType;
    using T = TargetType;

    // Display names are placeholders: change them here freely, nothing else depends on them.
    // (The bundled font is ASCII only, the intended Japanese names are in the comments.)
    //
    // The order must match SpellId.
    constexpr SpellData spells[] = {
        //  name              MP    type         target             power var hits chance
        // --- Hero: weapon techniques (power = % of a normal attack) ---
        { "Charge",           2,  { E::charge,   T::self,           0,    0 } },            // ためる
        { "Sweep",            4,  { E::physical, T::all_enemies,    80,   0 } },            // なぎ払い
        { "Mineuchi",         5,  { E::physical, T::one_enemy,      120,  0,  1,   50 } },  // みねうち (stuns)
        { "Full Slash",       8,  { E::physical, T::one_enemy,      200,  0 } },            // 全力斬り
        { "Ultima Slash",     16, { E::physical, T::one_enemy,      350,  0 } },            // 究極斬り
        // --- Sion: spear techniques ---
        { "Pierce",           3,  { E::physical, T::one_enemy,      130,  0 } },            // つきさす
        { "Double Thrust",    6,  { E::physical, T::one_enemy,      80,   0,  2 } },        // 連続突き
        { "Whirl Spear",      9,  { E::physical, T::all_enemies,    110,  0 } },            // 旋風槍
        { "Infinite Spear",   18, { E::physical, T::random_enemy,   85,   0,  4 } },        // 無限槍
        // --- Mirea: recovery magic ---
        { "Heal",             3,  { E::heal_hp,  T::one_ally,       30,   10 } },           // ヒール
        { "Behoimi",          6,  { E::heal_hp,  T::one_ally,       80,   20 } },           // ベホイミ
        { "Behoma",           10, { E::heal_hp,  T::one_ally,       9999, 0 } },            // ベホマ
        { "Zaoral",           12, { E::revive,   T::one_ally,       50,   0,  1,   50 } },  // ザオラル
        { "Full Restore",     30, { E::heal_hp,  T::all_allies,     9999, 0 } },            // 完全回復
        // --- Nox: attack magic (fixed damage) ---
        { "Mera",             2,  { E::damage,   T::one_enemy,      10,   6 } },            // メラ
        { "Merami",           6,  { E::damage,   T::one_enemy,      55,   15 } },           // メラミ
        { "Merazoma",         12, { E::damage,   T::one_enemy,      120,  30 } },           // メラゾーマ
        { "Doomfire",         28, { E::damage,   T::all_enemies,    220,  50 } },           // 終焉の炎
    };

    static_assert(sizeof(spells) / sizeof(spells[0]) == int(SpellId::count), "Spell table doesn't match SpellId");
}

const SpellData& spell_data(SpellId id)
{
    BN_ASSERT(id < SpellId::count, "Invalid spell id: ", int(id));

    return spells[int(id)];
}

}
