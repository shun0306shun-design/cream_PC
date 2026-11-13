#include "rpg_events.h"

#include "bn_string.h"
#include "bn_sstream.h"
#include "bn_sprite_ptr.h"

#include "bn_sprite_items_npc.h"

#include "rpg_game_state.h"

namespace rpg
{

namespace
{
    constexpr int inn_price = 10;
}

bool ask_yes_no(Ui& ui)
{
    static constexpr bn::string_view options[] = { "Yes", "No" };

    Menu menu(ui, yes_no_col, yes_no_row, yes_no_cols, options);
    return menu.run() == 0;
}

void run_talk(Ui& ui, const MapEvent& event)
{
    MessageBox box(ui);
    box.say_pages(event.pages);
}

void run_inn(GameState& state, Ui& ui, const MapEvent& event)
{
    MessageBox box(ui);
    box.say_pages(event.pages);

    bn::string<64> question;
    bn::ostringstream question_stream(question);
    question_stream << "A night costs " << inn_price << " gold.\nDo you want to rest?";
    box.show(question);

    if(! ask_yes_no(ui))
    {
        box.say("Come back any time.");
    }
    else if(state.party.gold < inn_price)
    {
        box.say("You don't have enough gold.");
    }
    else
    {
        state.party.gold -= inn_price;
        ui.fade_out();
        state.party.restore_all();
        ui.wait(30);
        ui.fade_in();
        box.say("The party is fully rested!");
    }
}

void use_spring(GameState& state, Ui& ui, const MapEvent& event)
{
    Party& party = state.party;
    MessageBox box(ui);
    box.say_pages(event.pages);

    // The spring heals the living only: reviving is what the inn is for.
    bool needs_healing = false;
    bool has_fallen = false;

    for(const PartyMember& member : party.members)
    {
        if(! member.alive())
        {
            has_fallen = true;
        }
        else
        {
            Stats stats = member.stats();
            needs_healing = needs_healing || member.hp < stats.max_hp || member.mp < stats.max_mp;
        }
    }

    if(! needs_healing)
    {
        box.say(has_fallen ? "The water can't revive\nfallen allies." : "Everyone is already in\ngood shape.");
        return;
    }

    bn::string<64> question;
    bn::ostringstream question_stream(question);
    question_stream << "Drink from the spring?";

    if(event.price > 0)
    {
        question_stream << "\nIt costs " << int(event.price) << " gold.";
    }

    box.show(question);

    if(! ask_yes_no(ui))
    {
        return;
    }

    if(party.gold < event.price)
    {
        box.say("You don't have enough gold.");
        return;
    }

    party.gold -= event.price;
    ui.fade_out();

    for(PartyMember& member : party.members)
    {
        if(member.alive())
        {
            member.restore();       // Full HP and MP.
        }
    }

    ui.wait(20);
    ui.fade_in();
    box.say(has_fallen ? "The party feels refreshed!\nBut the fallen stay down." : "The party feels refreshed!");
}

bool open_chest(GameState& state, Ui& ui, const MapEvent& event, bn::sprite_ptr* chest_sprite)
{
    BN_ASSERT(event.flag != FlagId::none, "A chest needs a flag id");

    Party& party = state.party;
    MessageBox box(ui);

    if(state.flags.get(event.flag))
    {
        box.say("The chest is empty.");
        return false;
    }

    // Don't waste the contents: leave the chest closed if the bag can't take the items.
    if(event.item_amount > 0 && party.item_amount(event.item) + event.item_amount > max_item_stack)
    {
        box.say("You can't carry any more\nof that item.");
        return false;
    }

    // Open it for good (the flag is saved with the game) and show the open sprite
    // before the messages, so the player sees it open while reading them.
    state.flags.set(event.flag);

    if(chest_sprite)
    {
        chest_sprite->set_tiles(bn::sprite_items::npc.tiles_item(), event.npc_graphics + 1);
    }

    bn::string<64> text;

    if(event.item_amount > 0)
    {
        party.add_item(event.item, event.item_amount);

        bn::ostringstream stream(text);
        stream << "Found " << item_data(event.item).name;

        if(event.item_amount > 1)
        {
            stream << " x" << int(event.item_amount);
        }

        stream << "!";
        box.say(text);
    }

    if(event.gold > 0)
    {
        party.gold += event.gold;

        text.clear();
        bn::ostringstream stream(text);
        stream << "Found " << int(event.gold) << " gold!";
        box.say(text);
    }

    if(event.item_amount == 0 && event.gold == 0)
    {
        box.say("It was empty...");
    }

    return true;
}

}
