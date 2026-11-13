#include "rpg_field_menu.h"

#include "bn_string.h"
#include "bn_sstream.h"

#include "rpg_ui.h"
#include "rpg_equip_menu.h"
#include "rpg_game_state.h"

namespace rpg
{

namespace
{
    using Label = bn::string<32>;

    // Layout, in cells. The submenus open at the right of the main menu.
    constexpr int main_cols = 10;
    constexpr int sub_col = main_cols;
    constexpr int sub_cols = screen_columns - sub_col;

    void say(Ui& ui, const bn::string_view& text)
    {
        MessageBox box(ui);
        box.say(text);
    }

    /// Returns the index of the chosen party member or -1.
    [[nodiscard]] int choose_member(const Party& party, Ui& ui)
    {
        Label labels[max_party_size];
        bn::string_view options[max_party_size];

        for(int index = 0; index < party.size; ++index)
        {
            const PartyMember& member = party.members[index];
            bn::ostringstream stream(labels[index]);
            stream << member.name() << "  HP " << member.hp << "/" << member.stats().max_hp;
            options[index] = labels[index];
        }

        Menu menu(ui, sub_col, 0, sub_cols, bn::span<const bn::string_view>(options, party.size));
        return menu.run();
    }

    /**
     * Uses a healing effect on the party, asking for the target if needed.
     * Returns true if it did something (so the item or the MP must be spent).
     */
    [[nodiscard]] bool use_heal_effect(const Effect& effect, GameState& state, Ui& ui)
    {
        Party& party = state.party;
        Label text;
        bn::ostringstream stream(text);
        int total = 0;

        if(effect.targets_all())
        {
            for(PartyMember& member : party.active())
            {
                total += apply_heal(effect, member, state.random);
            }

            stream << "The party recovers!";
        }
        else
        {
            int target = choose_member(party, ui);

            if(target < 0)
            {
                return false;
            }

            PartyMember& member = party.members[target];
            total = apply_heal(effect, member, state.random);
            stream << member.name() << " recovers " << total << (effect.type == EffectType::heal_hp ? " HP." : " MP.");
        }

        say(ui, total > 0 ? bn::string_view(text) : bn::string_view("It has no effect."));
        return total > 0;
    }

    void show_status(const GameState& state, Ui& ui)
    {
        while(true)
        {
            int index = choose_member(state.party, ui);

            if(index < 0)
            {
                return;
            }

            const PartyMember& member = state.party.members[index];
            Stats stats = member.stats();
            Window window(ui, sub_col, 0, sub_cols, Window::rows_for_lines(6));
            Label line;

            {
                bn::ostringstream stream(line);
                stream << member.name() << "   Lv " << member.level;
                window.print(0, line);
            }
            {
                line.clear();
                bn::ostringstream stream(line);
                stream << "HP " << member.hp << "/" << stats.max_hp << "   MP " << member.mp << "/" << stats.max_mp;
                window.print(1, line);
            }
            {
                line.clear();
                bn::ostringstream stream(line);
                stream << "ATK " << stats.attack << "  DEF " << stats.defense << "  AGI " << stats.agility;
                window.print(2, line);
            }
            {
                line.clear();
                bn::ostringstream stream(line);
                stream << "EXP " << member.exp << "   Next " << member.exp_to_next_level();
                window.print(3, line);
            }

            // Equipment: the item names, or "-" for an empty slot (same style as the equipment menu).
            for(int slot = 0; slot < 2; ++slot)
            {
                ItemId worn = member.equipped(slot == 0 ? ItemKind::weapon : ItemKind::armor);
                line.clear();
                bn::ostringstream stream(line);
                stream << (slot == 0 ? "W: " : "A: ") << (worn == ItemId::none ? "-" : item_data(worn).name);
                window.print(4 + slot, line);
            }

            ui.wait_for_a();
        }
    }

    void use_items(GameState& state, Ui& ui)
    {
        Party& party = state.party;

        while(true)
        {
            // List only the owned items.
            bn::vector<ItemId, item_count> ids;
            Label labels[item_count];
            bn::string_view options[item_count];

            for(int index = 0; index < item_count; ++index)
            {
                ItemId id = ItemId(index);

                if(int amount = party.item_amount(id); amount > 0 && item_data(id).kind == ItemKind::consumable)
                {
                    bn::ostringstream stream(labels[ids.size()]);
                    stream << item_data(id).name << "  x" << amount;
                    options[ids.size()] = labels[ids.size()];
                    ids.push_back(id);
                }
            }

            if(ids.empty())
            {
                say(ui, "You have no items.");
                return;
            }

            int choice;

            {
                Menu menu(ui, sub_col, 0, sub_cols, bn::span<const bn::string_view>(options, ids.size()), 6);
                choice = menu.run();
            }

            if(choice < 0)
            {
                return;
            }

            const ItemData& item = item_data(ids[choice]);

            if(! item.effect.is_heal())
            {
                say(ui, "You can't use that here.");
            }
            else if(use_heal_effect(item.effect, state, ui))
            {
                party.remove_item(ids[choice]);
            }
        }
    }

    void use_magic(GameState& state, Ui& ui)
    {
        Party& party = state.party;

        while(true)
        {
            int caster_index = choose_member(party, ui);

            if(caster_index < 0)
            {
                return;
            }

            PartyMember& caster = party.members[caster_index];

            // Only healing spells can be cast outside of battle.
            SpellList spells;
            Label labels[max_known_spells];
            bn::string_view options[max_known_spells];

            if(caster.alive())
            {
                for(SpellId id : caster.spells())
                {
                    const SpellData& spell = spell_data(id);

                    if(spell.effect.is_heal())
                    {
                        bn::ostringstream stream(labels[spells.size()]);
                        stream << spell.name << "  " << spell.mp_cost << " MP";
                        options[spells.size()] = labels[spells.size()];
                        spells.push_back(id);
                    }
                }
            }

            if(spells.empty())
            {
                say(ui, "No spells to cast here.");
                continue;
            }

            int choice;

            {
                Menu menu(ui, sub_col, 0, sub_cols, bn::span<const bn::string_view>(options, spells.size()), 6);
                choice = menu.run();
            }

            if(choice < 0)
            {
                continue;
            }

            const SpellData& spell = spell_data(spells[choice]);

            if(caster.mp < spell.mp_cost)
            {
                say(ui, "Not enough MP.");
            }
            else if(use_heal_effect(spell.effect, state, ui))
            {
                caster.mp = int16_t(caster.mp - spell.mp_cost);
            }
        }
    }
}

void open_field_menu(GameState& state, Ui& ui)
{
    static constexpr bn::string_view options[] = { "Status", "Items", "Magic", "Equip", "Save" };

    Menu menu(ui, 0, 0, main_cols, options, 5);

    while(true)
    {
        int choice;

        {
            // The gold window is only shown while the main menu has the focus,
            // because the message box needs that space.
            Window gold_window(ui, 0, Window::rows_for_lines(5), main_cols, Window::rows_for_lines(1));
            Label gold_text;
            bn::ostringstream gold_stream(gold_text);
            gold_stream << state.party.gold << " G";
            gold_window.print(0, gold_text);

            choice = menu.run();
        }

        switch(choice)
        {

        case 0:
            show_status(state, ui);
            break;

        case 1:
            use_items(state, ui);
            break;

        case 2:
            use_magic(state, ui);
            break;

        case 3:
            open_equip_menu(state, ui);
            break;

        case 4:
            save_game(state);
            say(ui, "Game saved.");
            break;

        default:
            return;
        }
    }
}

}
