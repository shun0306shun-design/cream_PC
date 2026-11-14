#include "rpg_spells.h"

#include "bn_assert.h"

#include "rpg_text.h"

namespace rpg
{

namespace
{
    using E = EffectType;
    using T = TargetType;

    // Display names are placeholders: change them here freely, nothing else depends on them
    // (after adding new kanji, run tools/gen_japanese_font.py).
    //
    // The order must match SpellId.
    constexpr SpellData spells[] = {
        //  name                                   MP    type         target             power var hits chance
        // --- The hero: weapon techniques (power = % of a normal attack) ---
        { RPG_T("Charge", "ためる"),               2,  { E::charge,   T::self,           0,    0 } },
        { RPG_T("Sweep", "なぎ払い"),              4,  { E::physical, T::all_enemies,    80,   0 } },
        { RPG_T("Mineuchi", "みねうち"),           5,  { E::physical, T::one_enemy,      120,  0,  1,   50 } },  // Stuns.
        { RPG_T("Full Slash", "全力斬り"),         8,  { E::physical, T::one_enemy,      200,  0 } },
        { RPG_T("Ultima Slash", "究極斬り"),       16, { E::physical, T::one_enemy,      350,  0 } },
        // --- The partner: spear techniques ---
        { RPG_T("Pierce", "つきさす"),             3,  { E::physical, T::one_enemy,      130,  0 } },
        { RPG_T("Double Thrust", "連続突き"),      6,  { E::physical, T::one_enemy,      80,   0,  2 } },
        { RPG_T("Whirl Spear", "旋風槍"),          9,  { E::physical, T::all_enemies,    110,  0 } },
        { RPG_T("Infinite Spear", "無限槍"),       18, { E::physical, T::random_enemy,   85,   0,  4 } },
        // --- The seer: recovery magic ---
        { RPG_T("Heal", "ヒール"),                 3,  { E::heal_hp,  T::one_ally,       30,   10 } },
        { RPG_T("Behoimi", "ベホイミ"),            6,  { E::heal_hp,  T::one_ally,       80,   20 } },
        { RPG_T("Behoma", "ベホマ"),               10, { E::heal_hp,  T::one_ally,       9999, 0 } },
        { RPG_T("Zaoral", "ザオラル"),             12, { E::revive,   T::one_ally,       50,   0,  1,   50 } },
        { RPG_T("Full Restore", "完全回復"),       30, { E::heal_hp,  T::all_allies,     9999, 0 } },
        // --- The former cult priest: attack magic (fixed damage) ---
        { RPG_T("Mera", "メラ"),                   2,  { E::damage,   T::one_enemy,      10,   6 } },
        { RPG_T("Merami", "メラミ"),               6,  { E::damage,   T::one_enemy,      55,   15 } },
        { RPG_T("Merazoma", "メラゾーマ"),         12, { E::damage,   T::one_enemy,      120,  30 } },
        { RPG_T("Doomfire", "終焉の炎"),           28, { E::damage,   T::all_enemies,    220,  50 } },
        // --- Enemies only (the story bosses are balanced around these two) ---
        { RPG_T("Fire", "ファイア"),               0,  { E::damage,   T::one_enemy,      10,   6 } },
        { RPG_T("Blizzard", "ブリザード"),         0,  { E::damage,   T::all_enemies,    9,    5 } },
    };

    static_assert(sizeof(spells) / sizeof(spells[0]) == int(SpellId::count), "Spell table doesn't match SpellId");
}

const SpellData& spell_data(SpellId id)
{
    BN_ASSERT(id < SpellId::count, "Invalid spell id: ", int(id));

    return spells[int(id)];
}

}
