#include "rpg_story.h"

#include "bn_assert.h"

#include "rpg_ui.h"
#include "rpg_names.h"
#include "rpg_game_state.h"

/*
 * The script of the main story. Every scene is a short function: show pages, change the progress, maybe start a
 * boss battle. All the text is constexpr (ROM); names come from rpg_names.h.
 *
 * Text limits: 3 lines per page (the box holds 4), about 28 characters per line.
 */

namespace rpg
{

namespace
{
    // Shorter names for the dialogue below.
    #define HERO RPG_NAME_HERO
    #define PARTNER RPG_NAME_PARTNER
    #define GENERAL RPG_NAME_GENERAL_1

    void say_pages(Ui& ui, bn::span<const bn::string_view> pages)
    {
        MessageBox box(ui);
        box.say_pages(pages);
    }

    // ---------------------------------------------------------------------------------------------------------------
    // Chapter 1 "Morning of Ash"
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view opening_pages[] = {
        RPG_NAME_VILLAGE " Village.\nA quiet morning,\nlike any other.",
        "Mother: " HERO "! Your father\nis calling you from\nthe south gate.",
    };

    constexpr bn::string_view father_pages[] = {
        "Father: Morning, " HERO ".\nFetch some water from the\nspring, will you?",
        "Father: It's in the north-\neast corner. Your mother's\nmaking stew tonight.",
    };

    constexpr bn::string_view father_again_pages[] = {
        "Father: The spring is in\nthe northeast corner.\nGo on, don't dawdle.",
    };

    constexpr bn::string_view attack_pages[] = {
        "You fill the pail. The\nwater is cold and clear.",
        "...Smoke?\nThe sky over the village\nis turning black.",
        "Men in ash-gray armor pour\nthrough the gate, torches\nin their hands.",
        "Father: " HERO ", run!\nTake your mother and--",
        "A blade wrapped in flame.\nFather falls. Mother shields\nyou... and falls, too.",
        "Soldier: Burn it all.\nFor the Flame Emperor.",
        "Something strikes your\nhead. The world goes dark.",
    };

    constexpr bn::string_view awakening_pages[] = {
        "Three days later.",
        RPG_NAME_VILLAGE " is gone. Only ash\nand the smell of smoke\nremain.",
        "Two fresh graves stand\nwhere the square used\nto be.",
    };

    constexpr bn::string_view gate_peaceful_pages[] = {
        HERO ": Not now.\nFather asked me for\nsomething first.",
    };

    constexpr bn::string_view gate_ashes_pages[] = {
        HERO ": ...Not yet.\nI should speak to\nthe Elder first.",
    };

    constexpr bn::string_view elder_pages[] = {
        "Elder: You're awake, " HERO ".\nI buried them myself.\nForgive an old man.",
        "Elder: The ones who did\nthis wore a crest. A crown\nwreathed in ash.",
        "Elder: One of their scouts\nstill hides in the Watch\nCave, in the rocks south.",
        HERO ": ...Then that is\nwhere I start.",
        "Elder: Take your father's\nsword. And come back\nalive, boy.",
        "Got Father's " "Bronze Sword!",
    };

    constexpr bn::string_view scout_pages[] = {
        "Cult Scout: A survivor?\nFrom that little village?",
        HERO ": You burned " RPG_NAME_VILLAGE ".\nWho gave the order?",
        "Cult Scout: Ash asks no\nquestions, boy. Burn!",
    };

    constexpr bn::string_view scout_victory_pages[] = {
        "The scout falls. A crest\nslips from his cloak:\na crown wreathed in ash.",
        "Cult Scout: Heh... General\n" GENERAL " rides for " RPG_NAME_TOWN ".\nYou're already too late.",
        HERO ": The " RPG_NAME_CULT ".\n" GENERAL ". ..." RPG_NAME_TOWN ", then.",
        "- Chapter 2 -\nBlades That Never Cross",
    };

    constexpr EncounterGroup scout_battle = { { EnemyId::cult_scout }, 1 };

    // ---------------------------------------------------------------------------------------------------------------
    // Chapter 2 "Blades That Never Cross"
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view road_pages[] = {
        "The road east leads to\n" RPG_NAME_TOWN ". There is something\nto finish here first.",
    };

    constexpr bn::string_view partner_town_pages[] = {
        "A spearman watches you\nfrom under a traveler's\nhood.",
        "???: You've been asking\nabout " GENERAL ". Stop.",
        HERO ": He burned my\nvillage. I'm going to\nkill him.",
        PARTNER ": Name's " PARTNER ". His\nbounty is mine, farm boy.",
        PARTNER ": Revenge makes you\nslow. Slow gets you\nkilled.",
        HERO ": Stay out of my way.",
        PARTNER ": He camps in the old\nfort, east of here. Try not\nto die before I get paid.",
        PARTNER " leaves through\nthe east gate.",
    };

    constexpr bn::string_view east_gate_pages[] = {
        "The old highway.\nNothing calls you\nthat way yet.",
    };

    constexpr bn::string_view fort_pages[] = {
        PARTNER " is on one knee.\nA man in red armor\nstands over him.",
        GENERAL ": A bounty hunter\nand a stray. What small\nfoes the Emperor has.",
        HERO ": " GENERAL "!!",
        PARTNER ": Tch. You again...\nHe's too much for one.",
        PARTNER ": Fine. Just this\nonce. Keep up, " HERO "!",
        PARTNER " fights at\nyour side!",
    };

    constexpr bn::string_view fort_again_pages[] = {
        PARTNER ": Again! And don't\nfall behind this time!",
    };

    constexpr bn::string_view fort_victory_pages[] = {
        GENERAL ": ...Enough. You have\nearned the Emperor's eye.\nPray you earn no more.",
        GENERAL " vanishes in a\npillar of flame.",
        PARTNER ": He got away.\nSo did my bounty.",
        PARTNER ": ...You fight angry.\nBut you didn't run.",
        PARTNER ": I owe you one, and\nI hate owing. I'm coming\nalong until it's repaid.",
        HERO ": ...Do as you like.",
        PARTNER " joined the party!",
        "- Chapter 3 -",
    };

    constexpr EncounterGroup fort_battle = { { EnemyId::drake, EnemyId::cult_soldier }, 2 };

    constexpr int partner_min_level = 4;

    /// The partner joins at the hero's level, already armed.
    void join_partner(Party& party)
    {
        int level = party.members[0].level;

        if(party.join(ClassId::lancer, level < partner_min_level ? partner_min_level : level))
        {
            PartyMember& partner = party.members[party.size - 1];
            partner.weapon = ItemId::iron_spear;
            partner.restore();
        }
    }
}

StoryResult run_story(StoryId id, GameState& state, Ui& ui)
{
    StoryResult result;
    uint8_t& story_progress = state.story_progress;

    switch(id)
    {

    case StoryId::ch1_opening:
        say_pages(ui, opening_pages);
        story_progress = progress::ch1_morning;
        break;

    case StoryId::ch1_father:
        if(story_progress < progress::ch1_errand)
        {
            say_pages(ui, father_pages);
            story_progress = progress::ch1_errand;
        }
        else
        {
            say_pages(ui, father_again_pages);
        }
        break;

    case StoryId::ch1_attack:
        say_pages(ui, attack_pages);
        ui.fade_out(60);
        ui.wait(60);

        // Wake up in front of the graves, in the burnt village (another state of the same map).
        story_progress = progress::ch1_ashes;
        state.party.restore_all();
        state.player_x = 11;
        state.player_y = 9;
        state.facing = Direction::up;
        result.reload_map = true;
        result.next = StoryId::ch1_awakening;
        break;

    case StoryId::ch1_awakening:
        say_pages(ui, awakening_pages);
        break;

    case StoryId::ch1_gate:
        say_pages(ui, story_progress < progress::ch1_ashes ? bn::span<const bn::string_view>(gate_peaceful_pages) :
                                                             bn::span<const bn::string_view>(gate_ashes_pages));
        result.step_back = true;
        break;

    case StoryId::ch1_elder:
        {
            say_pages(ui, elder_pages);

            // The keepsake goes straight to the hero's hand if it is empty.
            Party& party = state.party;
            party.add_item(ItemId::bronze_sword);

            if(party.members[0].weapon == ItemId::none)
            {
                [[maybe_unused]] EquipResult equip_result = party.equip(party.members[0], ItemId::bronze_sword);
            }

            story_progress = progress::ch1_departed;
        }
        break;

    case StoryId::ch1_scout:
        say_pages(ui, scout_pages);
        result.battle = &scout_battle;
        break;

    case StoryId::ch2_road:
        say_pages(ui, road_pages);
        result.step_back = true;
        break;

    case StoryId::ch2_partner_town:
        say_pages(ui, partner_town_pages);
        story_progress = progress::ch2_rivals;
        break;

    case StoryId::ch2_east_gate:
        say_pages(ui, east_gate_pages);
        result.step_back = true;
        break;

    case StoryId::ch2_fort:
        if(story_progress < progress::ch2_allied)
        {
            say_pages(ui, fort_pages);
            join_partner(state.party);
            story_progress = progress::ch2_allied;
        }
        else
        {
            say_pages(ui, fort_again_pages);    // After losing the battle: no need to repeat it all.
        }

        result.battle = &fort_battle;
        break;

    default:
        BN_ERROR("Invalid story id: ", int(id));
        break;
    }

    return result;
}

void run_story_victory(StoryId id, GameState& state, Ui& ui)
{
    switch(id)
    {

    case StoryId::ch1_scout:
        say_pages(ui, scout_victory_pages);
        state.story_progress = progress::ch2_start;
        break;

    case StoryId::ch2_fort:
        // The general is only driven off. The partner, who fought as a guest, stays for good.
        say_pages(ui, fort_victory_pages);
        state.story_progress = progress::ch3_start;
        break;

    default:
        break;
    }
}

}
