#include "rpg_equip_menu.h"

#include "bn_string.h"
#include "bn_vector.h"
#include "bn_sstream.h"

#include "rpg_ui.h"
#include "rpg_text.h"
#include "rpg_game_state.h"

namespace rpg
{

namespace
{
    using Label = bn::string<64>;      // Room for UTF-8 Japanese (3 bytes per character).

    // Same area as the other field submenus: right of the main menu. Everything stays above row 12,
    // where the message box (used for the result messages) starts.
    constexpr int sub_col = 10;
    constexpr int sub_cols = screen_columns - sub_col;
    constexpr int list_lines = 6;

    static_assert(Window::rows_for_lines(list_lines) + 1 <= screen_rows - MessageBox::box_rows, "List too tall");

    /// Writes the stat changes of an item: "ATK+5 DEF+2" (in Japanese, one kanji per stat).
    void print_bonus(bn::ostringstream& stream, const Stats& bonus)
    {
        bool first = true;

        auto add = [&](const bn::string_view& name, int value)
        {
            if(value != 0)
            {
                stream << (first ? "" : " ") << name << (value > 0 ? "+" : "") << value;
                first = false;
            }
        };

        add("HP", bonus.max_hp);
        add("MP", bonus.max_mp);
        add(tr("ATK", "攻"), bonus.attack);
        add(tr("DEF", "守"), bonus.defense);
        add(tr("AGI", "速"), bonus.agility);
    }

    /// Returns the index of the chosen member or -1.
    [[nodiscard]] int choose_member(const Party& party, Ui& ui)
    {
        bn::string_view options[max_party_size];

        for(int index = 0; index < party.size; ++index)
        {
            options[index] = party.members[index].name();
        }

        Menu menu(ui, sub_col, 0, sub_cols, bn::span<const bn::string_view>(options, party.size));
        return menu.run();
    }

    /// Lets the player change what a member wears in a slot.
    void change_slot(GameState& state, Ui& ui, PartyMember& member, ItemKind slot)
    {
        Party& party = state.party;

        // Options: "Remove" (only if something is worn), then the matching items in the bag.
        bn::vector<ItemId, item_count + 1> ids;     // ItemId::none stands for "Remove".
        Label labels[item_count + 1];
        bn::string_view options[item_count + 1];
        ItemId worn = member.equipped(slot);

        if(worn != ItemId::none)
        {
            labels[0] = RPG_T("(Take off)", "（はずす）");
            options[0] = labels[0];
            ids.push_back(ItemId::none);
        }

        for(int index = 0; index < item_count; ++index)
        {
            ItemId id = ItemId(index);
            const ItemData& item = item_data(id);

            if(item.kind == slot && (item.classes & class_bit(member.class_id)) && party.item_amount(id) > 0)
            {
                bn::ostringstream stream(labels[ids.size()]);
                stream << localized(item.name) << "  ";
                print_bonus(stream, item.bonus);
                options[ids.size()] = labels[ids.size()];
                ids.push_back(id);
            }
        }

        MessageBox box(ui);

        if(ids.empty())
        {
            box.say(RPG_T("Nothing to equip.", "装備できるものがない。"));
            return;
        }

        int choice;

        {
            Menu menu(ui, sub_col, 0, sub_cols, bn::span<const bn::string_view>(options, ids.size()), list_lines);
            choice = menu.run();
        }

        if(choice < 0)
        {
            return;
        }

        ItemId id = ids[choice];
        EquipResult result = id == ItemId::none ? party.unequip(member, slot) : party.equip(member, id);
        Label text;
        bn::ostringstream stream(text);

        if(result == EquipResult::ok)
        {
            if(id == ItemId::none)
            {
                stream << member.name() << tr(" took off\n", "は\n") << localized(item_data(worn).name) << tr(".", "をはずした。");
            }
            else
            {
                stream << member.name() << tr(" equipped\n", "は\n") << localized(item_data(id).name) << tr(".", "を装備した。");
            }
        }
        else
        {
            stream << tr("Your bag is full.", "道具袋がいっぱいだ。");      // The other results can't happen with this menu.
        }

        box.say(text);
    }

    /// The slots and stats of one member: shown until the player goes back.
    void edit_member(GameState& state, Ui& ui, PartyMember& member)
    {
        int last_slot = 0;

        while(true)
        {
            int choice;

            {
                // Rebuilt every time so the names and stats reflect the last change.
                Label labels[2];
                bn::string_view options[2];

                for(int index = 0; index < 2; ++index)
                {
                    ItemId worn = member.equipped(index == 0 ? ItemKind::weapon : ItemKind::armor);
                    bn::ostringstream stream(labels[index]);
                    stream << (index == 0 ? tr("W: ", "武器: ") : tr("A: ", "防具: ")) << (worn == ItemId::none ? bn::string_view("-") :
                                                                                            localized(item_data(worn).name));
                    options[index] = labels[index];
                }

                Menu slot_menu(ui, sub_col, 0, sub_cols, options);
                slot_menu.set_index(last_slot);

                Window stats_window(ui, sub_col, Window::rows_for_lines(2), sub_cols, Window::rows_for_lines(2));
                Stats stats = member.stats();
                Label line;

                {
                    bn::ostringstream stream(line);
                    stream << member.name() << "   Lv " << member.level;
                    stats_window.print(0, line);
                }
                {
                    line.clear();
                    bn::ostringstream stream(line);
                    stream << tr("ATK ", "攻 ") << stats.attack << tr("  DEF ", "  守 ") << stats.defense
                           << tr("  AGI ", "  速 ") << stats.agility;
                    stats_window.print(1, line);
                }

                choice = slot_menu.run();
            }

            if(choice < 0)
            {
                return;
            }

            last_slot = choice;
            change_slot(state, ui, member, choice == 0 ? ItemKind::weapon : ItemKind::armor);
        }
    }
}

void open_equip_menu(GameState& state, Ui& ui)
{
    int last_member = 0;

    while(true)
    {
        int index = choose_member(state.party, ui);

        if(index < 0)
        {
            return;
        }

        last_member = index;
        edit_member(state, ui, state.party.members[last_member]);
    }
}

}
