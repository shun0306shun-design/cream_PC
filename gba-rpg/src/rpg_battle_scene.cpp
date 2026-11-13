#include "rpg_battle_scene.h"

#include "bn_sstream.h"
#include "bn_sprite_item.h"

#include "rpg_game_state.h"

namespace rpg
{

namespace
{
    using Text = bn::string<96>;
    using Label = bn::string<32>;

    // Screen layout, in cells:
    //   rows 0-6:   party status window
    //   rows 7-11:  enemies
    //   rows 12-19: command menu (left) + submenus (right), or the message box
    constexpr int status_rows = Window::rows_for_lines(3);
    constexpr int bottom_row = screen_rows - MessageBox::box_rows;
    constexpr int command_cols = 9;
    constexpr int sub_col = command_cols;
    constexpr int sub_cols = screen_columns - command_cols;
    constexpr int status_column_width = (screen_width - 16) / max_party_size;     // In pixels.

    constexpr int enemy_spacing = 56;
    constexpr int enemy_y = (((status_rows + bottom_row) * 8) / 2) - (screen_height / 2);
}

BattleScene::BattleScene(GameState& state, Ui& ui, const EncounterGroup& group, bool can_escape) :
    _state(state),
    _ui(ui),
    _party(state.party),
    _status_window(ui, 0, 0, screen_columns, status_rows),
    _can_escape(can_escape)
{
    BN_ASSERT(group.count >= 1 && group.count <= max_enemies, "Invalid enemies count: ", group.count);

    for(int index = 0; index < group.count; ++index)
    {
        const EnemyData& data = enemy_data(group.enemies[index]);

        // Enemies of the same kind get a letter: "Slime A", "Slime B"...
        int same_kind = 0;
        int same_kind_before = 0;

        for(int other = 0; other < group.count; ++other)
        {
            if(group.enemies[other] == group.enemies[index])
            {
                ++same_kind;

                if(other < index)
                {
                    ++same_kind_before;
                }
            }
        }

        bn::string<16> name(data.name);

        if(same_kind > 1)
        {
            name.push_back(' ');
            name.push_back(char('A' + same_kind_before));
        }

        // Enemies are spread horizontally around the center of the screen.
        int x = ((index * 2) - (group.count - 1)) * (enemy_spacing / 2);
        bn::sprite_ptr sprite = data.sprite->create_sprite(x, enemy_y);
        sprite.set_bg_priority(scene_sprite_bg_priority);
        _enemies.push_back(Enemy{ &data, data.stats.max_hp, name, bn::move(sprite) });
    }

    _refresh_status();
}

BattleResult BattleScene::run()
{
    _ui.fade_in();

    BattleResult result = _run_battle();

    _ui.fade_out();
    return result;
}

BattleResult BattleScene::_run_battle()
{
    {
        MessageBox box(_ui);
        Text text;
        bn::ostringstream stream(text);

        if(_enemies.size() == 1)
        {
            stream << _enemies[0].name << " appears!";
        }
        else
        {
            stream << _enemies[0].name << " and company\nappear!";
        }

        box.say(text);
    }

    while(true)
    {
        // 1) The player chooses what every party member will do.
        bool wants_to_run = _select_commands();

        // 2) Everyone acts.
        MessageBox box(_ui);

        if(wants_to_run && _try_escape(box))
        {
            return BattleResult::escaped;
        }

        _execute_round(box);

        // 3) Check how the round ended.
        if(_all_enemies_dead())
        {
            _victory(box);
            return BattleResult::victory;
        }

        if(_party.all_dead())
        {
            box.say("The party was wiped out...");
            return BattleResult::defeat;
        }
    }
}

void BattleScene::_refresh_status()
{
    _status_window.clear_text();

    for(int index = 0; index < _party.size; ++index)
    {
        const PartyMember& member = _party.members[index];
        int x = index * status_column_width;
        Label line;

        _status_window.print(0, member.name(), x);

        {
            bn::ostringstream stream(line);
            stream << "H " << member.hp;
            _status_window.print(1, line, x);
        }
        {
            line.clear();
            bn::ostringstream stream(line);
            stream << "M " << member.mp;
            _status_window.print(2, line, x);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Command selection
// ---------------------------------------------------------------------------------------------------------------------

bool BattleScene::_select_commands()
{
    for(Action& action : _actions)
    {
        action = Action();
    }

    int member_index = 0;

    while(member_index < _party.size)
    {
        if(! _party.members[member_index].alive())
        {
            ++member_index;
            continue;
        }

        switch(_select_command(member_index))
        {

        case CommandResult::confirmed:
            ++member_index;
            break;

        case CommandResult::back:
            // Go back to the previous member that can act, if any.
            for(int previous = member_index - 1; previous >= 0; --previous)
            {
                if(_party.members[previous].alive())
                {
                    member_index = previous;
                    break;
                }
            }
            break;

        case CommandResult::run:
            for(Action& action : _actions)
            {
                action = Action();
            }
            return true;

        default:
            break;
        }
    }

    return false;
}

BattleScene::CommandResult BattleScene::_select_command(int member_index)
{
    static constexpr bn::string_view commands[] = { "Fight", "Magic", "Item", "Run" };

    const PartyMember& member = _party.members[member_index];
    Action& action = _actions[member_index];
    Menu command_menu(_ui, 0, bottom_row, command_cols, commands);

    while(true)
    {
        int command;

        {
            // Shown only while the command menu has the focus: the submenus use the same space.
            Window info(_ui, sub_col, bottom_row, sub_cols, MessageBox::box_rows);
            info.print(0, member.name());
            info.print(1, "What will you do?");
            command = command_menu.run();
        }

        switch(command)
        {

        case 0:
            action.type = ActionType::attack;
            action.target = int8_t(_choose_enemy());

            if(action.target >= 0)
            {
                return CommandResult::confirmed;
            }
            break;

        case 1:
            if(_choose_spell(member_index, action))
            {
                return CommandResult::confirmed;
            }
            break;

        case 2:
            if(_choose_item(action))
            {
                return CommandResult::confirmed;
            }
            break;

        case 3:
            return CommandResult::run;

        default:
            return CommandResult::back;
        }
    }
}

bool BattleScene::_choose_spell(int member_index, Action& action)
{
    const PartyMember& member = _party.members[member_index];
    SpellList spells = member.spells();

    if(spells.empty())
    {
        _notice("No spells learned yet.");
        return false;
    }

    Label labels[max_known_spells];
    bn::string_view options[max_known_spells];

    for(int index = 0; index < spells.size(); ++index)
    {
        const SpellData& spell = spell_data(spells[index]);
        bn::ostringstream stream(labels[index]);
        stream << spell.name << "  " << spell.mp_cost << " MP";
        options[index] = labels[index];
    }

    while(true)
    {
        int choice;

        {
            Menu menu(_ui, sub_col, bottom_row, sub_cols, bn::span<const bn::string_view>(options, spells.size()));
            choice = menu.run();
        }

        if(choice < 0)
        {
            return false;
        }

        const SpellData& spell = spell_data(spells[choice]);

        if(member.mp < spell.mp_cost)
        {
            _notice("Not enough MP.");
        }
        else if(_choose_target(spell.effect, action))
        {
            action.type = ActionType::spell;
            action.id = uint8_t(spells[choice]);
            return true;
        }
    }
}

bool BattleScene::_choose_item(Action& action)
{
    bn::vector<ItemId, item_count> ids;
    Label labels[item_count];
    bn::string_view options[item_count];

    for(int index = 0; index < item_count; ++index)
    {
        ItemId id = ItemId(index);

        if(int amount = _party.item_amount(id); amount > 0 && item_data(id).kind == ItemKind::consumable)
        {
            bn::ostringstream stream(labels[ids.size()]);
            stream << item_data(id).name << "  x" << amount;
            options[ids.size()] = labels[ids.size()];
            ids.push_back(id);
        }
    }

    if(ids.empty())
    {
        _notice("You have no items.");
        return false;
    }

    while(true)
    {
        int choice;

        {
            Menu menu(_ui, sub_col, bottom_row, sub_cols, bn::span<const bn::string_view>(options, ids.size()));
            choice = menu.run();
        }

        if(choice < 0)
        {
            return false;
        }

        if(_choose_target(item_data(ids[choice]).effect, action))
        {
            action.type = ActionType::item;
            action.id = uint8_t(ids[choice]);
            return true;
        }
    }
}

bool BattleScene::_choose_target(const Effect& effect, Action& action)
{
    if(effect.targets_all())
    {
        action.target = -1;
        return true;
    }

    int target = effect.is_heal() ? _choose_ally() : _choose_enemy();

    if(target < 0)
    {
        return false;
    }

    action.target = int8_t(target);
    return true;
}

int BattleScene::_choose_enemy()
{
    // Only the enemies still standing can be chosen.
    bn::vector<int, max_enemies> indexes;
    bn::string_view options[max_enemies];

    for(int index = 0; index < _enemies.size(); ++index)
    {
        if(_enemies[index].alive())
        {
            options[indexes.size()] = _enemies[index].name;
            indexes.push_back(index);
        }
    }

    Menu menu(_ui, sub_col, bottom_row, sub_cols, bn::span<const bn::string_view>(options, indexes.size()));
    int choice = menu.run();
    return choice < 0 ? -1 : indexes[choice];
}

int BattleScene::_choose_ally()
{
    bn::string_view options[max_party_size];

    for(int index = 0; index < _party.size; ++index)
    {
        options[index] = _party.members[index].name();
    }

    Menu menu(_ui, sub_col, bottom_row, sub_cols, bn::span<const bn::string_view>(options, _party.size));
    return menu.run();
}

void BattleScene::_notice(const bn::string_view& text)
{
    Window window(_ui, sub_col, bottom_row, sub_cols, MessageBox::box_rows);
    window.print_text(0, text);
    _ui.wait_for_a();
}

// ---------------------------------------------------------------------------------------------------------------------
// Round execution
// ---------------------------------------------------------------------------------------------------------------------

void BattleScene::_execute_round(MessageBox& box)
{
    // Build the turn order: higher agility usually acts first, with some randomness.
    bn::vector<Turn, max_party_size + max_enemies> turns;
    bn::random& random = _state.random;

    for(int index = 0; index < _party.size; ++index)
    {
        const PartyMember& member = _party.members[index];

        if(member.alive() && _actions[index].type != ActionType::none)
        {
            int agility = member.stats().agility;
            turns.push_back(Turn{ false, int8_t(index), int16_t(agility + random.get_int((agility / 2) + 4)) });
        }
    }

    for(int index = 0; index < _enemies.size(); ++index)
    {
        if(_enemies[index].alive())
        {
            int agility = _enemies[index].data->stats.agility;
            turns.push_back(Turn{ true, int8_t(index), int16_t(agility + random.get_int((agility / 2) + 4)) });
        }
    }

    // Insertion sort: there are 7 turns at most.
    for(int index = 1; index < turns.size(); ++index)
    {
        Turn turn = turns[index];
        int position = index;

        while(position > 0 && turns[position - 1].initiative < turn.initiative)
        {
            turns[position] = turns[position - 1];
            --position;
        }

        turns[position] = turn;
    }

    for(const Turn& turn : turns)
    {
        if(_all_enemies_dead() || _party.all_dead())
        {
            return;
        }

        if(turn.is_enemy)
        {
            _enemy_turn(turn.index, box);
        }
        else
        {
            _party_turn(turn.index, box);
        }
    }
}

void BattleScene::_party_turn(int member_index, MessageBox& box)
{
    PartyMember& member = _party.members[member_index];
    const Action& action = _actions[member_index];

    if(! member.alive())
    {
        return;     // Fell before acting.
    }

    Text text;
    bn::ostringstream stream(text);

    switch(action.type)
    {

    case ActionType::none:
        break;

    case ActionType::attack:
        {
            int target = _valid_enemy_target(action.target);

            if(target >= 0)
            {
                stream << member.name() << " attacks!";
                box.say_timed(text);

                int defense = _enemies[target].data->stats.defense;
                _damage_enemy(target, roll_physical_damage(member.stats().attack, defense, _state.random), box);
            }
        }
        break;

    case ActionType::spell:
        {
            const SpellData& spell = spell_data(SpellId(action.id));
            stream << member.name() << " casts " << spell.name << "!";
            box.say_timed(text);

            member.mp = int16_t(member.mp - spell.mp_cost);
            _refresh_status();
            _apply_party_effect(spell.effect, action.target, box);
        }
        break;

    case ActionType::item:
        {
            ItemId id = ItemId(action.id);
            const ItemData& item = item_data(id);

            // Two members may have chosen the last unit of the same item.
            if(_party.remove_item(id))
            {
                stream << member.name() << " uses " << item.name << "!";
                box.say_timed(text);
                _apply_party_effect(item.effect, action.target, box);
            }
            else
            {
                stream << member.name() << " looks for " << item.name << ",\nbut there are none left!";
                box.say_timed(text);
            }
        }
        break;

    default:
        break;
    }
}

void BattleScene::_enemy_turn(int enemy_index, MessageBox& box)
{
    // Enemy AI: cast its spell with some chance, attack someone otherwise.
    // Replace this with per-enemy behaviour tables for smarter enemies.
    Enemy& enemy = _enemies[enemy_index];

    if(! enemy.alive())
    {
        return;
    }

    const EnemyData& data = *enemy.data;
    Text text;
    bn::ostringstream stream(text);

    if(data.spell_chance > 0 && _state.random.get_int(100) < data.spell_chance)
    {
        const SpellData& spell = spell_data(data.spell);
        stream << enemy.name << " casts " << spell.name << "!";
        box.say_timed(text);
        _apply_enemy_effect(spell.effect, enemy_index, box);
    }
    else
    {
        int target = _random_alive_member();
        stream << enemy.name << " attacks!";
        box.say_timed(text);

        int defense = _party.members[target].stats().defense;
        _damage_member(target, roll_physical_damage(data.stats.attack, defense, _state.random), box);
    }
}

void BattleScene::_apply_party_effect(const Effect& effect, int target, MessageBox& box)
{
    if(effect.is_heal())
    {
        if(effect.targets_all())
        {
            for(int index = 0; index < _party.size; ++index)
            {
                if(_party.members[index].alive())
                {
                    _heal_member(index, effect, box);
                }
            }
        }
        else
        {
            _heal_member(target, effect, box);
        }
    }
    else if(effect.targets_all())
    {
        for(int index = 0; index < _enemies.size(); ++index)
        {
            if(_enemies[index].alive())
            {
                _damage_enemy(index, roll_effect(effect, _state.random), box);
            }
        }
    }
    else
    {
        int enemy_index = _valid_enemy_target(target);

        if(enemy_index >= 0)
        {
            _damage_enemy(enemy_index, roll_effect(effect, _state.random), box);
        }
    }
}

void BattleScene::_apply_enemy_effect(const Effect& effect, int enemy_index, MessageBox& box)
{
    // Same effects as the party, seen from the other side.
    if(effect.is_heal())
    {
        if(effect.type != EffectType::heal_hp)
        {
            return;     // Enemies don't use MP.
        }

        if(effect.targets_all())
        {
            for(int index = 0; index < _enemies.size(); ++index)
            {
                if(_enemies[index].alive())
                {
                    _heal_enemy(index, roll_effect(effect, _state.random), box);
                }
            }
        }
        else
        {
            _heal_enemy(enemy_index, roll_effect(effect, _state.random), box);
        }
    }
    else if(effect.targets_all())
    {
        for(int index = 0; index < _party.size; ++index)
        {
            if(_party.members[index].alive())
            {
                _damage_member(index, roll_effect(effect, _state.random), box);
            }
        }
    }
    else
    {
        _damage_member(_random_alive_member(), roll_effect(effect, _state.random), box);
    }
}

void BattleScene::_damage_enemy(int enemy_index, int amount, MessageBox& box)
{
    Enemy& enemy = _enemies[enemy_index];
    enemy.hp = int16_t(amount >= enemy.hp ? 0 : enemy.hp - amount);

    // Hit feedback: blink the sprite.
    for(int blink = 0; blink < 6; ++blink)
    {
        enemy.sprite.set_visible(blink % 2);
        _ui.wait(3);
    }

    Text text;
    bn::ostringstream stream(text);
    stream << enemy.name << " takes " << amount << " damage!";

    if(! enemy.alive())
    {
        enemy.sprite.set_visible(false);
        stream << "\n" << enemy.name << " is defeated!";
    }

    box.say_timed(text);
}

void BattleScene::_damage_member(int member_index, int amount, MessageBox& box)
{
    PartyMember& member = _party.members[member_index];
    member.take_damage(amount);
    _refresh_status();

    Text text;
    bn::ostringstream stream(text);
    stream << member.name() << " takes " << amount << " damage!";

    if(! member.alive())
    {
        stream << "\n" << member.name() << " falls!";
    }

    box.say_timed(text);
}

void BattleScene::_heal_member(int member_index, const Effect& effect, MessageBox& box)
{
    PartyMember& member = _party.members[member_index];
    Text text;
    bn::ostringstream stream(text);

    if(member.alive())
    {
        int amount = apply_heal(effect, member, _state.random);
        _refresh_status();
        stream << member.name() << " recovers " << amount << (effect.type == EffectType::heal_hp ? " HP." : " MP.");
    }
    else
    {
        stream << "It has no effect on " << member.name() << ".";
    }

    box.say_timed(text);
}

void BattleScene::_heal_enemy(int enemy_index, int amount, MessageBox& box)
{
    Enemy& enemy = _enemies[enemy_index];
    int missing = enemy.data->stats.max_hp - enemy.hp;

    if(amount > missing)
    {
        amount = missing;
    }

    enemy.hp = int16_t(enemy.hp + amount);

    Text text;
    bn::ostringstream stream(text);
    stream << enemy.name << " recovers " << amount << " HP.";
    box.say_timed(text);
}

bool BattleScene::_try_escape(MessageBox& box)
{
    box.say_timed("The party tries to run away...");

    if(! _can_escape)
    {
        box.say_timed("There is no escape\nfrom this fight!");
        return false;
    }

    if(_state.random.get_int(100) < 60)
    {
        box.say("Got away safely!");
        return true;
    }

    box.say_timed("But the enemies block the way!");
    return false;
}

void BattleScene::_victory(MessageBox& box)
{
    int exp = 0;
    int gold = 0;

    for(const Enemy& enemy : _enemies)
    {
        exp += enemy.data->exp;
        gold += enemy.data->gold;
    }

    _party.gold += gold;

    {
        Text text;
        bn::ostringstream stream(text);
        stream << "You won the battle!\nGained " << exp << " EXP and " << gold << " gold.";
        box.say(text);
    }

    // Every survivor gets the full experience.
    for(PartyMember& member : _party.active())
    {
        if(! member.alive())
        {
            continue;
        }

        int old_level = member.level;

        if(member.add_exp(exp) > 0)
        {
            _refresh_status();

            Text text;
            bn::ostringstream stream(text);
            stream << member.name() << " reached level " << member.level << "!";

            for(const SpellLearn& learn : class_data(member.class_id).spells)
            {
                if(learn.level > old_level && learn.level <= member.level)
                {
                    stream << "\nLearned " << spell_data(learn.spell).name << "!";
                }
            }

            box.say(text);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------------------------------------------------

bool BattleScene::_all_enemies_dead() const
{
    for(const Enemy& enemy : _enemies)
    {
        if(enemy.alive())
        {
            return false;
        }
    }

    return true;
}

int BattleScene::_valid_enemy_target(int enemy_index) const
{
    // If the chosen enemy was already defeated this round, attack the first one standing.
    if(enemy_index >= 0 && enemy_index < _enemies.size() && _enemies[enemy_index].alive())
    {
        return enemy_index;
    }

    for(int index = 0; index < _enemies.size(); ++index)
    {
        if(_enemies[index].alive())
        {
            return index;
        }
    }

    return -1;
}

int BattleScene::_random_alive_member() const
{
    bn::vector<int, max_party_size> indexes;

    for(int index = 0; index < _party.size; ++index)
    {
        if(_party.members[index].alive())
        {
            indexes.push_back(index);
        }
    }

    BN_ASSERT(! indexes.empty(), "There are no party members alive");

    return indexes[_state.random.get_int(indexes.size())];
}

}
