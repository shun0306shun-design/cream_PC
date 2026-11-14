#include "rpg_party.h"

#include "bn_assert.h"
#include "bn_random.h"

namespace rpg
{

int roll_effect(const Effect& effect, bn::random& random)
{
    return effect.power + random.get_int(effect.variance + 1);
}

int roll_physical_damage(int attack, int defense, bn::random& random)
{
    int base = (attack / 2) - (defense / 4);

    if(base < 1)
    {
        // Weak attackers still scratch: 0 or 1 points.
        return random.get_int(2);
    }

    // +-12.5% variance.
    int spread = (base / 8) + 1;
    int damage = base - spread + random.get_int((spread * 2) + 1);
    return damage < 1 ? 1 : damage;
}

Stats PartyMember::stats() const
{
    Stats result = stats_for_level(class_id, level);

    for(ItemId id : { weapon, armor })
    {
        if(id != ItemId::none)
        {
            const Stats& bonus = item_data(id).bonus;
            result.max_hp = int16_t(result.max_hp + bonus.max_hp);
            result.max_mp = int16_t(result.max_mp + bonus.max_mp);
            result.attack = int16_t(result.attack + bonus.attack);
            result.defense = int16_t(result.defense + bonus.defense);
            result.agility = int16_t(result.agility + bonus.agility);
        }
    }

    return result;
}

ItemId PartyMember::equipped(ItemKind slot) const
{
    BN_ASSERT(slot != ItemKind::consumable, "Not an equipment slot");

    return slot == ItemKind::weapon ? weapon : armor;
}

bool PartyMember::valid() const
{
    // Equipment ids must exist and be in the right slot: item_data() would assert otherwise.
    auto slot_valid = [](ItemId id, ItemKind slot)
    {
        return id == ItemId::none || (id < ItemId::count && item_data(id).kind == slot);
    };

    if(class_id >= ClassId::count || level < 1 || level > max_level ||
       ! slot_valid(weapon, ItemKind::weapon) || ! slot_valid(armor, ItemKind::armor))
    {
        return false;
    }

    // Too much HP/MP is not an error: load_game() rounds it down to the maximum (for example after the
    // equipment bonuses were rebalanced). Negative values can't come from the game.
    return hp >= 0 && mp >= 0;
}

void PartyMember::clamp_to_stats()
{
    Stats current_stats = stats();

    if(hp > current_stats.max_hp)
    {
        hp = current_stats.max_hp;
    }

    if(mp > current_stats.max_mp)
    {
        mp = current_stats.max_mp;
    }
}

void PartyMember::init(ClassId new_class_id, int new_level)
{
    BN_ASSERT(new_level >= 1 && new_level <= max_level, "Invalid level: ", new_level);

    class_id = new_class_id;
    level = uint8_t(new_level);
    exp = exp_for_level(new_level);
    restore();
}

int PartyMember::exp_to_next_level() const
{
    if(level >= max_level)
    {
        return 0;
    }

    return exp_for_level(level + 1) - exp;
}

SpellList PartyMember::spells() const
{
    SpellList result;

    for(const SpellLearn& learn : class_data(class_id).spells)
    {
        if(learn.level <= level && ! result.full())
        {
            result.push_back(learn.spell);
        }
    }

    return result;
}

void PartyMember::restore()
{
    Stats current_stats = stats();
    hp = current_stats.max_hp;
    mp = current_stats.max_mp;
}

int PartyMember::take_damage(int amount)
{
    if(amount > hp)
    {
        amount = hp;
    }

    hp = int16_t(hp - amount);
    return amount;
}

int PartyMember::heal_hp(int amount)
{
    int missing = stats().max_hp - hp;

    if(amount > missing)
    {
        amount = missing;
    }

    hp = int16_t(hp + amount);
    return amount;
}

int PartyMember::heal_mp(int amount)
{
    int missing = stats().max_mp - mp;

    if(amount > missing)
    {
        amount = missing;
    }

    mp = int16_t(mp + amount);
    return amount;
}

int PartyMember::add_exp(int amount)
{
    int gained_levels = 0;
    exp += amount;

    while(level < max_level && exp >= exp_for_level(level + 1))
    {
        // Current HP/MP grow by the same amount as their maximum.
        Stats old_stats = stats();
        ++level;
        ++gained_levels;

        Stats new_stats = stats();
        hp = int16_t(hp + new_stats.max_hp - old_stats.max_hp);
        mp = int16_t(mp + new_stats.max_mp - old_stats.max_mp);
    }

    return gained_levels;
}

void Party::init_new_game()
{
    // The hero starts alone: the others join with the story (see Party::join() and rpg_story.cpp).
    for(PartyMember& member : members)
    {
        member = PartyMember();
    }

    members[0].init(ClassId::hero, 1);
    size = 1;

    for(uint8_t& item : items)
    {
        item = 0;
    }

    add_item(ItemId::herb, 5);
    add_item(ItemId::magic_water, 1);
    add_item(ItemId::fire_bomb, 2);
    gold = 50;
}

bool Party::join(ClassId class_id, int level)
{
    if(size >= max_party_size)
    {
        return false;
    }

    for(const PartyMember& member : active())
    {
        if(member.class_id == class_id)
        {
            return false;
        }
    }

    members[size] = PartyMember();
    members[size].init(class_id, level);
    ++size;
    return true;
}

bool Party::leave(ClassId class_id)
{
    for(int index = 1; index < size; ++index)
    {
        if(members[index].class_id == class_id)
        {
            // Keep the equipment: it returns to the bag (if the bag has room for it).
            for(ItemId id : { members[index].weapon, members[index].armor })
            {
                if(id != ItemId::none)
                {
                    add_item(id);
                }
            }

            for(int other = index; other < size - 1; ++other)
            {
                members[other] = members[other + 1];
            }

            --size;
            members[size] = PartyMember();
            return true;
        }
    }

    return false;
}

bool Party::valid() const
{
    if(size < 1 || size > max_party_size)
    {
        return false;
    }

    for(const PartyMember& member : active())
    {
        if(! member.valid())
        {
            return false;
        }
    }

    return true;
}

bool Party::all_dead() const
{
    for(const PartyMember& member : active())
    {
        if(member.alive())
        {
            return false;
        }
    }

    return true;
}

void Party::restore_all()
{
    for(PartyMember& member : active())
    {
        member.restore();
    }
}

void Party::add_item(ItemId id, int amount)
{
    int new_amount = items[int(id)] + amount;
    items[int(id)] = uint8_t(new_amount > max_item_stack ? max_item_stack : new_amount);
}

bool Party::remove_item(ItemId id)
{
    if(items[int(id)] == 0)
    {
        return false;
    }

    --items[int(id)];
    return true;
}

EquipResult Party::equip(PartyMember& member, ItemId id)
{
    const ItemData& data = item_data(id);

    if(! data.is_equipment() || ! (data.classes & class_bit(member.class_id)))
    {
        return EquipResult::not_allowed;
    }

    if(item_amount(id) == 0)
    {
        return EquipResult::not_owned;
    }

    ItemId& slot = data.kind == ItemKind::weapon ? member.weapon : member.armor;
    ItemId old_item = slot;

    // Check before changing anything: the old item has to fit in the bag.
    if(old_item != ItemId::none && old_item != id && item_amount(old_item) >= max_item_stack)
    {
        return EquipResult::bag_full;
    }

    remove_item(id);

    if(old_item != ItemId::none)
    {
        add_item(old_item);
    }

    slot = id;
    member.clamp_to_stats();
    return EquipResult::ok;
}

EquipResult Party::unequip(PartyMember& member, ItemKind slot_kind)
{
    ItemId& slot = slot_kind == ItemKind::weapon ? member.weapon : member.armor;

    if(slot == ItemId::none)
    {
        return EquipResult::ok;
    }

    if(item_amount(slot) >= max_item_stack)
    {
        return EquipResult::bag_full;
    }

    add_item(slot);
    slot = ItemId::none;
    member.clamp_to_stats();
    return EquipResult::ok;
}

int apply_heal(const Effect& effect, PartyMember& target, bn::random& random)
{
    if(! target.alive())
    {
        return 0;
    }

    int amount = roll_effect(effect, random);

    switch(effect.type)
    {

    case EffectType::heal_hp:
        return target.heal_hp(amount);

    case EffectType::heal_mp:
        return target.heal_mp(amount);

    default:
        return 0;
    }
}

bool apply_revive(const Effect& effect, PartyMember& target, bn::random& random)
{
    BN_ASSERT(effect.type == EffectType::revive, "Not a revive effect");
    BN_ASSERT(! target.alive(), "The target is not fallen");

    if(random.get_int(100) >= effect.chance)
    {
        return false;
    }

    int hp = (target.stats().max_hp * effect.power) / 100;
    target.hp = int16_t(hp < 1 ? 1 : hp);
    return true;
}

}
