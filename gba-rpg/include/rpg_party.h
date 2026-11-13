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
    ItemId weapon = ItemId::none;       //!< Equipped weapon (an item of kind weapon) or ItemId::none.
    ItemId armor = ItemId::none;        //!< Equipped armor or ItemId::none.

    /// Sets class and level, with full HP and MP.
    void init(ClassId new_class_id, int new_level);

    [[nodiscard]] bn::string_view name() const
    {
        return class_data(class_id).name;
    }

    /// Class and level stats plus the bonus of the equipment. Battles and menus always use this.
    [[nodiscard]] Stats stats() const;

    /// The equipped item of a slot (ItemKind::weapon or ItemKind::armor), or ItemId::none.
    [[nodiscard]] ItemId equipped(ItemKind slot) const;

    /// Checks that the data (for example loaded from a save) can't make the game crash.
    [[nodiscard]] bool valid() const;

    /// Lowers HP/MP to the current maximums (after taking equipment off).
    void clamp_to_stats();

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

enum class EquipResult : uint8_t
{
    ok,
    not_owned,      //!< There's none of that item in the bag.
    not_allowed,    //!< Not equipment, or this class can't use it.
    bag_full        //!< The item to put back in the bag is at the stack limit.
};

struct Party
{
    PartyMember members[max_party_size];
    uint8_t items[max_items] = {};      //!< Amount owned of each item, indexed by ItemId (the rest is unused).
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

    /// Equips an item from the bag. The item that was in that slot goes back to the bag.
    [[nodiscard]] EquipResult equip(PartyMember& member, ItemId id);

    /// Takes off the item of a slot (it goes to the bag). Does nothing if the slot is empty.
    [[nodiscard]] EquipResult unequip(PartyMember& member, ItemKind slot);
};

/**
 * Applies a healing effect (heal_hp / heal_mp) to a member and returns the points restored.
 * Fallen members can't be healed (returns 0).
 */
int apply_heal(const Effect& effect, PartyMember& target, bn::random& random);

}

#endif
