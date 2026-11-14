#include "rpg_shop.h"

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

    // Layout, in cells (the message box takes the bottom 8 rows, so nothing here goes past row 11):
    //   command menu (Buy / Sell / Exit) and gold at the left, item list at the right.
    constexpr int command_cols = 9;
    constexpr int command_rows = Window::rows_for_lines(3);
    constexpr int list_col = command_cols;
    constexpr int list_cols = screen_columns - list_col;
    constexpr int list_lines = 6;                           // Visible items: the list scrolls after that.

    static_assert(Window::rows_for_lines(list_lines) + MessageBox::box_rows <= screen_rows, "Shop list too tall");
    static_assert(command_rows + Window::rows_for_lines(1) + MessageBox::box_rows <= screen_rows, "Shop menu too tall");

    void print_gold(Window& window, int gold)
    {
        Label text;
        bn::ostringstream stream(text);
        stream << gold << " G";

        window.clear_text();
        window.print(0, text);
    }

    void buy(GameState& state, Window& gold_window, MessageBox& box, const bn::span<const ItemId>& stock)
    {
        BN_ASSERT(stock.size() <= item_count, "Too many items in a shop: ", stock.size());

        if(stock.empty())
        {
            box.say(RPG_T("Sorry, we're sold out.", "すまない、売り切れだ。"));
            return;
        }

        // Option texts are built once: the stock doesn't change while the player is buying.
        Label labels[item_count];
        bn::string_view options[item_count];

        for(int index = 0; index < stock.size(); ++index)
        {
            const ItemData& item = item_data(stock[index]);
            bn::ostringstream stream(labels[index]);
            stream << localized(item.name) << "  " << item.price << " G";
            options[index] = labels[index];
        }

        Party& party = state.party;
        Menu menu(box.ui(), list_col, 0, list_cols, bn::span<const bn::string_view>(options, stock.size()), list_lines);

        while(true)
        {
            box.show(RPG_T("What would you like to buy?", "何を買うんだい？"));
            int choice = menu.run();

            if(choice < 0)
            {
                return;
            }

            ItemId id = stock[choice];
            const ItemData& item = item_data(id);

            // Always check before changing anything: gold first, then room in the bag.
            if(party.gold < item.price)
            {
                box.say(RPG_T("You don't have enough gold.", "お金が足りないよ。"));
            }
            else if(party.item_amount(id) >= max_item_stack)
            {
                box.say(RPG_T("You can't carry any more\nof that item.", "それはもう持てないよ。"));
            }
            else
            {
                party.gold -= item.price;
                party.add_item(id);
                print_gold(gold_window, party.gold);
                box.say(RPG_T("Thank you!", "まいどあり！"));
            }
        }
    }

    void sell(GameState& state, Window& gold_window, MessageBox& box)
    {
        Party& party = state.party;
        int last_choice = 0;

        while(true)
        {
            // The list is rebuilt after every sale: amounts change and items can run out.
            bn::vector<ItemId, item_count> ids;
            Label labels[item_count];
            bn::string_view options[item_count];

            for(int index = 0; index < item_count; ++index)
            {
                ItemId id = ItemId(index);

                if(int amount = party.item_amount(id))
                {
                    bn::ostringstream stream(labels[ids.size()]);
                    stream << localized(item_data(id).name) << " x" << amount << "  " << sell_price(id) << " G";
                    options[ids.size()] = labels[ids.size()];
                    ids.push_back(id);
                }
            }

            if(ids.empty())
            {
                box.say(RPG_T("You have nothing to sell.", "売れるものがないね。"));
                return;
            }

            box.show(RPG_T("What would you like to sell?", "何を売るんだい？"));
            int choice;

            {
                Menu menu(box.ui(), list_col, 0, list_cols, bn::span<const bn::string_view>(options, ids.size()),
                          list_lines);

                // Keep the cursor where it was, instead of jumping back to the first item.
                menu.set_index(last_choice < ids.size() ? last_choice : ids.size() - 1);
                choice = menu.run();
            }

            if(choice < 0)
            {
                return;
            }

            last_choice = choice;

            ItemId id = ids[choice];
            int price = sell_price(id);

            if(price <= 0)
            {
                box.say(RPG_T("Sorry, I can't buy that.", "すまない、それは\n買い取れないんだ。"));
            }
            else if(party.remove_item(id))
            {
                party.gold += price;
                print_gold(gold_window, party.gold);
                box.say(RPG_T("Thank you!", "まいどあり！"));
            }
        }
    }
}

int sell_price(ItemId id)
{
    return (item_data(id).price * sell_percent) / 100;
}

void open_shop(GameState& state, Ui& ui, const bn::span<const ItemId>& stock,
               const bn::span<const bn::string_view>& greeting)
{
    static constexpr bn::string_view commands[] = { RPG_T("Buy", "買う"), RPG_T("Sell", "売る"), RPG_T("Exit", "やめる") };

    MessageBox box(ui);

    if(greeting.empty())
    {
        box.say(RPG_T("Welcome!", "いらっしゃい！"));
    }
    else
    {
        box.say_pages(greeting);
    }

    {
        // These windows stay open while the buy/sell list is shown next to them.
        Menu command_menu(ui, 0, 0, command_cols, commands);
        Window gold_window(ui, 0, command_rows, command_cols, Window::rows_for_lines(1));
        print_gold(gold_window, state.party.gold);

        while(true)
        {
            box.show(RPG_T("What can I do for you?", "何の用だい？"));

            int command = command_menu.run();

            if(command == 0)
            {
                buy(state, gold_window, box, stock);
            }
            else if(command == 1)
            {
                sell(state, gold_window, box);
            }
            else
            {
                break;
            }
        }
    }

    box.say(RPG_T("Thank you! Come again!", "まいどあり！ またどうぞ！"));
}

}
