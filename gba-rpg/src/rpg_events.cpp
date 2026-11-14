#include "rpg_events.h"

#include "bn_string.h"
#include "bn_sstream.h"
#include "bn_sprite_ptr.h"

#include "bn_sprite_items_npc.h"

#include "rpg_audio.h"
#include "rpg_game_state.h"

namespace rpg
{

namespace
{
    constexpr int inn_price = 10;
}

bool ask_yes_no(Ui& ui)
{
    static constexpr bn::string_view options[] = { RPG_T("Yes", "はい"), RPG_T("No", "いいえ") };

    Menu menu(ui, yes_no_col, yes_no_row, yes_no_cols, options);
    return menu.run() == 0;
}

void run_talk(const GameState& state, Ui& ui, const MapEvent& event)
{
    MessageBox box(ui);
    box.say_pages(event.pages_for(state.story_progress));
}

void run_inn(GameState& state, Ui& ui, const MapEvent& event)
{
    MessageBox box(ui);
    box.say_pages(event.pages);

    // Japanese takes 3 bytes per character (UTF-8): text built at run time needs room for it.
    bn::string<128> question;
    bn::ostringstream question_stream(question);

    if(language() == Language::english)
    {
        question_stream << "A night costs " << inn_price << " gold.\nDo you want to rest?";
    }
    else
    {
        question_stream << "一晩 " << inn_price << "ゴールドです。\nお泊まりになりますか？";
    }

    box.show(question);

    if(! ask_yes_no(ui))
    {
        box.say(RPG_T("Come back any time.", "またどうぞ。"));
    }
    else if(state.party.gold < inn_price)
    {
        box.say(RPG_T("You don't have enough gold.", "お金が足りない。"));
    }
    else
    {
        state.party.gold -= inn_price;
        ui.fade_out();
        state.party.restore_all();
        ui.wait(30);
        ui.fade_in();
        box.say(RPG_T("The party is fully rested!", "みんな、すっかり元気になった！"));
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

    for(const PartyMember& member : party.active())
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
        box.say(has_fallen ? RPG_T("The water can't revive\nfallen allies.",
                                   "この水では、たおれた仲間は\n目をさまさない。") :
                             RPG_T("Everyone is already in\ngood shape.",
                                   "みんな元気いっぱいだ。"));
        return;
    }

    bn::string<128> question;
    bn::ostringstream question_stream(question);
    question_stream << tr("Drink from the spring?", "泉の水を飲みますか？");

    if(event.price > 0)
    {
        question_stream << tr("\nIt costs ", "\n") << int(event.price) << tr(" gold.", "ゴールドかかります。");
    }

    box.show(question);

    if(! ask_yes_no(ui))
    {
        return;
    }

    if(party.gold < event.price)
    {
        box.say(RPG_T("You don't have enough gold.", "お金が足りない。"));
        return;
    }

    party.gold -= event.price;
    ui.fade_out();

    for(PartyMember& member : party.active())
    {
        if(member.alive())
        {
            member.restore();       // Full HP and MP.
        }
    }

    ui.wait(20);
    ui.fade_in();
    box.say(has_fallen ? RPG_T("The party feels refreshed!\nBut the fallen stay down.",
                               "みんなの体力が回復した！\nだが、たおれた仲間は\nそのままだ。") :
                         RPG_T("The party feels refreshed!",
                               "みんなの体力が回復した！"));
}

bool open_chest(GameState& state, Ui& ui, const MapEvent& event, bn::sprite_ptr* chest_sprite)
{
    BN_ASSERT(event.flag != FlagId::none, "A chest needs a flag id");

    Party& party = state.party;
    MessageBox box(ui);

    if(state.flags.get(event.flag))
    {
        box.say(RPG_T("The chest is empty.", "宝箱は空っぽだ。"));
        return false;
    }

    // Don't waste the contents: leave the chest closed if the bag can't take the items.
    if(event.item_amount > 0 && party.item_amount(event.item) + event.item_amount > max_item_stack)
    {
        box.say(RPG_T("You can't carry any more\nof that item.", "これ以上は持てない。"));
        return false;
    }

    // Open it for good (the flag is saved with the game) and show the open sprite
    // before the messages, so the player sees it open while reading them.
    state.flags.set(event.flag);
    audio::play_se(SeId::chest);

    if(chest_sprite)
    {
        chest_sprite->set_tiles(bn::sprite_items::npc.tiles_item(), event.npc_graphics + 1);
    }

    bn::string<128> text;      // Room for UTF-8 Japanese (3 bytes per character).

    if(event.item_amount > 0)
    {
        party.add_item(event.item, event.item_amount);

        bn::ostringstream stream(text);
        stream << tr("Found ", "") << localized(item_data(event.item).name);

        if(event.item_amount > 1)
        {
            stream << " x" << int(event.item_amount);
        }

        stream << tr("!", " を\n手に入れた！");
        box.say(text);
    }

    if(event.gold > 0)
    {
        party.gold += event.gold;

        text.clear();
        bn::ostringstream stream(text);
        stream << tr("Found ", "") << int(event.gold) << tr(" gold!", "ゴールドを手に入れた！");
        box.say(text);
    }

    if(event.item_amount == 0 && event.gold == 0)
    {
        box.say(RPG_T("It was empty...", "中は空っぽだった……。"));
    }

    return true;
}

}
