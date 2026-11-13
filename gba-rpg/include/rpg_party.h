/*
 * Runtime state of the party: members, inventory and gold.
 * Everything here is plain data (trivially copyable) so it can be written to SRAM as is.
 */

#ifndef RPG_PARTY_H
#define RPG_PARTY_H

#include "bn_vector.h"

#include "rpg_classes.h"
#include "rpg_items.h"

namespace rpg
{

constexpr int max_known_spells = 8;

using SpellList = bn::vector<SpellId, max_known_spells>;

struct PartyMember
{
    ClassId class_id = ClassId::hero;
    uint8_t level = 1;
    int16_t hp = 0;
    int16_t mp = 0;
    int32_t exp = 0;

    /// Sets class and level, with full HP and MP.
    void init(ClassId new_class_id, int new_level);

    [[nodiscard]] bn::string_view name() const
    {
        return class_data(class_id).name;
    }

    [[nodiscard]] Stats stats() const
    {
        return stats_for_level(class_id, level);
    }

    [[nodiscard]] bool alive() const
    {
        return hp > 0;
    }

    /// Experience still needed for the next level (0 at max level).
    [[nodiscard]] int exp_to_next_level() const;

    /// Spells known at the current level.
    [[nodiscard]] SpellList spells() const;

    /// Restores HP and MP (also revives).
    void restore();

    /// All of these return the amount really applied.
    int take_damage(int amount);
    int heal_hp(int amount);
    int heal_mp(int amount);

    /// Adds experience and returns how many levels were gained.
    int add_exp(int amount);
};

struct Party
{
    PartyMember members[max_party_size];
    uint8_t items[item_count] = {};     //!< Amount owned of each item, indexed by ItemId.
    int32_t gold = 0;

    void init_new_game();

    [[nodiscard]] bool all_dead() const;

    void restore_all();

    [[nodiscard]] int item_amount(ItemId id) const
    {
        return items[int(id)];
    }

    void add_item(ItemId id, int amount = 1);

    /// Returns false if there's no item to remove.
    bool remove_item(ItemId id);
};

/**
 * Applies a healing effect (heal_hp / heal_mp) to a member and returns the points restored.
 * Fallen members can't be healed (returns 0).
 */
int apply_heal(const Effect& effect, PartyMember& target, bn::random& random);

}

#endif
