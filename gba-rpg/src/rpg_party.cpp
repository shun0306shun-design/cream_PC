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
    // Starting party: edit here to change the line-up.
    members[0].init(ClassId::hero, 1);
    members[1].init(ClassId::samurai, 1);
    members[2].init(ClassId::priest, 1);
    members[3].init(ClassId::mage, 1);

    for(uint8_t& item : items)
    {
        item = 0;
    }

    add_item(ItemId::herb, 5);
    add_item(ItemId::magic_water, 1);
    add_item(ItemId::fire_bomb, 2);
    gold = 50;
}

bool Party::all_dead() const
{
    for(const PartyMember& member : members)
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
    for(PartyMember& member : members)
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
