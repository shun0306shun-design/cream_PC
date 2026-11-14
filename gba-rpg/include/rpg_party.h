/*
 * Runtime state of the party: members, inventory and gold.
 * Everything here is plain data (trivially copyable) so it can be written to SRAM as is.
 */

#ifndef RPG_PARTY_H
#define RPG_PARTY_H

#include "bn_span.h"
#include "bn_vector.h"

#include "rpg_text.h"
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

    /// The name in the current language (class names are RPG_T texts).
    [[nodiscard]] bn::string_view name() const
    {
        return localized(class_data(class_id).name);
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
    PartyMember members[max_party_size];    //!< Only the first `size` are in the party.
    uint8_t size = 1;                       //!< Members in the party (1..max_party_size). They join with the story.
    uint8_t items[max_items] = {};      //!< Amount owned of each item, indexed by ItemId (the rest is unused).
    int32_t gold = 0;

    void init_new_game();

    /// The members that are in the party.
    [[nodiscard]] bn::span<PartyMember> active()
    {
        return bn::span<PartyMember>(members, size);
    }

    [[nodiscard]] bn::span<const PartyMember> active() const
    {
        return bn::span<const PartyMember>(members, size);
    }

    /// Adds a member with full HP/MP. Returns false if the party is full or that class is already in it.
    bool join(ClassId class_id, int level);

    /// Removes the member of that class (never the first one). What it wears goes back to the bag.
    /// Returns false if nobody of that class is in the party.
    bool leave(ClassId class_id);

    /// Checks that the data (for example loaded from a save) can't make the game crash.
    [[nodiscard]] bool valid() const;

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

/**
 * Tries a revive effect on a fallen member: it comes back with `power` percent of its HP.
 * Returns false if the roll fails (see Effect::chance).
 */
bool apply_revive(const Effect& effect, PartyMember& target, bn::random& random);

}

#endif
