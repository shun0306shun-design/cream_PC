#include "rpg_story.h"

#include "bn_assert.h"

#include "rpg_ui.h"
#include "rpg_text.h"
#include "rpg_names.h"
#include "rpg_game_state.h"

/*
 * The script of the main story. Every scene is a short function: show pages, change the progress, maybe start a
 * boss battle. All the text is constexpr (ROM); names come from rpg_names.h.
 *
 * Every page is written in both languages with RPG_T(english, japanese), see rpg_text.h.
 *
 * Text limits: 3 lines per page (the box holds 4).
 * English: about 28 characters per line. Japanese: 18 full width characters per line.
 * After using new kanji, run tools/gen_japanese_font.py.
 */

namespace rpg
{

namespace
{
    // Shorter names for the dialogue below.
    #define HERO RPG_NAME_HERO
    #define PARTNER RPG_NAME_PARTNER
    #define GENERAL RPG_NAME_GENERAL_1
    #define SEER RPG_NAME_SEER
    #define GATE RPG_NAME_GATE
    #define HERO_JA RPG_NAME_HERO_JA
    #define PARTNER_JA RPG_NAME_PARTNER_JA
    #define GENERAL_JA RPG_NAME_GENERAL_1_JA
    #define VILLAGE_JA RPG_NAME_VILLAGE_JA
    #define TOWN_JA RPG_NAME_TOWN_JA

    void say_pages(Ui& ui, bn::span<const bn::string_view> pages)
    {
        MessageBox box(ui);
        box.say_pages(pages);
    }

    // ---------------------------------------------------------------------------------------------------------------
    // Chapter 1 "Morning of Ash"
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view opening_pages[] = {
        RPG_T(RPG_NAME_VILLAGE " Village.\nA quiet morning,\nlike any other.",
              VILLAGE_JA "村。\nいつもと変わらない、\n静かな朝だった。"),
        RPG_T("Mother: " HERO "! Your father\nis calling you from\nthe south gate.",
              "母「" HERO_JA "！\n　父さんが南の門で\n　呼んでいるわよ。」"),
    };

    constexpr bn::string_view father_pages[] = {
        RPG_T("Father: Morning, " HERO ".\nFetch some water from the\nspring, will you?",
              "父「おはよう、" HERO_JA "。\n　泉で水をくんできて\n　くれないか。」"),
        RPG_T("Father: It's in the north-\neast corner. Your mother's\nmaking stew tonight.",
              "父「泉は村の北東のすみだ。\n　今夜は母さんの\n　シチューだぞ。」"),
    };

    constexpr bn::string_view father_again_pages[] = {
        RPG_T("Father: The spring is in\nthe northeast corner.\nGo on, don't dawdle.",
              "父「泉は北東のすみだ。\n　さあ、道草をくうなよ。」"),
    };

    constexpr bn::string_view attack_pages[] = {
        RPG_T("You fill the pail. The\nwater is cold and clear.",
              "おけに水をくんだ。\n冷たく、すんだ水だ。"),
        RPG_T("...Smoke?\nThe sky over the village\nis turning black.",
              "……けむり？\n村の空が、\n黒くそまっていく。"),
        RPG_T("Men in ash-gray armor pour\nthrough the gate, torches\nin their hands.",
              "灰色のよろいの男たちが、\nたいまつを手に\n門からなだれこんでくる。"),
        RPG_T("Father: " HERO ", run!\nTake your mother and--",
              "父「" HERO_JA "、にげろ！\n　母さんをつれて――」"),
        RPG_T("A blade wrapped in flame.\nFather falls. Mother shields\nyou... and falls, too.",
              "炎をまとった刃。\n父がたおれる。母はきみを\nかばい……そして、たおれた。"),
        RPG_T("Soldier: Burn it all.\nFor the Flame Emperor.",
              "兵士「すべて焼きはらえ。\n　炎帝さまのために。」"),
        RPG_T("Something strikes your\nhead. The world goes dark.",
              "頭に強いしょうげきが走る。\n目の前が、暗くなった。"),
    };

    constexpr bn::string_view awakening_pages[] = {
        RPG_T("Three days later.",
              "三日後。"),
        RPG_T(RPG_NAME_VILLAGE " is gone. Only ash\nand the smell of smoke\nremain.",
              VILLAGE_JA "村は、もうない。\n残っているのは灰と、\nけむりのにおいだけだ。"),
        RPG_T("Two fresh graves stand\nwhere the square used\nto be.",
              "広場だった場所に、\n新しい墓が二つ\nならんでいる。"),
    };

    constexpr bn::string_view gate_peaceful_pages[] = {
        RPG_T(HERO ": Not now.\nFather asked me for\nsomething first.",
              HERO_JA "「今はだめだ。\n　先に父さんのたのみを\n　すませないと。」"),
    };

    constexpr bn::string_view gate_ashes_pages[] = {
        RPG_T(HERO ": ...Not yet.\nI should speak to\nthe Elder first.",
              HERO_JA "「……まだだ。\n　先に長老と\n　話しておこう。」"),
    };

    constexpr bn::string_view elder_pages[] = {
        RPG_T("Elder: You're awake, " HERO ".\nI buried them myself.\nForgive an old man.",
              "長老「目がさめたか、" HERO_JA "。\n　二人はわしがとむらった。\n　この老いぼれをゆるせ。」"),
        RPG_T("Elder: The ones who did\nthis wore a crest. A crown\nwreathed in ash.",
              "長老「やつらは紋章を\n　つけておった。\n　灰をまとった王冠じゃ。」"),
        RPG_T("Elder: One of their scouts\nstill hides in the Watch\nCave, in the rocks south.",
              "長老「南の岩山にある\n　見張りの洞窟に、やつらの\n　斥候がまだひそんでおる。」"),
        RPG_T(HERO ": ...Then that is\nwhere I start.",
              HERO_JA "「……なら、\n　そこから始める。」"),
        RPG_T("Elder: Take your father's\nsword. And come back\nalive, boy.",
              "長老「父の剣を持っていけ。\n　かならず、生きて\n　帰ってくるのじゃぞ。」"),
        RPG_T("Got Father's " "Bronze Sword!",
              "父の剣 (Bronze Sword) を\n手に入れた！"),
    };

    constexpr bn::string_view scout_pages[] = {
        RPG_T("Cult Scout: A survivor?\nFrom that little village?",
              "教団の斥候「生き残りだと？\n　あのちっぽけな村の？」"),
        RPG_T(HERO ": You burned " RPG_NAME_VILLAGE ".\nWho gave the order?",
              HERO_JA "「" VILLAGE_JA "を焼いたな。\n　命じたのはだれだ。」"),
        RPG_T("Cult Scout: Ash asks no\nquestions, boy. Burn!",
              "教団の斥候「灰は何も\n　問わぬ。燃えろ、小僧！」"),
    };

    constexpr bn::string_view scout_victory_pages[] = {
        RPG_T("The scout falls. A crest\nslips from his cloak:\na crown wreathed in ash.",
              "斥候がたおれた。マントから\n紋章がこぼれ落ちる。\n灰をまとった王冠だ。"),
        RPG_T("Cult Scout: Heh... General\n" GENERAL " rides for " RPG_NAME_TOWN ".\nYou're already too late.",
              "教団の斥候「へっ……\n　" GENERAL_JA "将軍は" TOWN_JA "へ\n　向かった。もう手おくれだ。」"),
        RPG_T(HERO ": The " RPG_NAME_CULT ".\n" GENERAL ". ..." RPG_NAME_TOWN ", then.",
              HERO_JA "「" RPG_NAME_CULT_JA "。\n　" GENERAL_JA "。\n　……次は、" TOWN_JA "か。」"),
        RPG_T("- Chapter 2 -\nBlades That Never Cross",
              "- 第2章 -\n交わらぬ刃"),
    };

    constexpr EncounterGroup scout_battle = { { EnemyId::cult_scout }, 1 };

    // ---------------------------------------------------------------------------------------------------------------
    // Chapter 2 "Blades That Never Cross"
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view road_pages[] = {
        RPG_T("The road east leads to\n" RPG_NAME_TOWN ". There is something\nto finish here first.",
              "東の道は" TOWN_JA "へ続いている。\nだが、ここでやり残した\nことがある。"),
    };

    constexpr bn::string_view partner_town_pages[] = {
        RPG_T("A spearman watches you\nfrom under a traveler's\nhood.",
              "旅人のフードの下から、\n槍使いがこちらを\n見ている。"),
        RPG_T("???: You've been asking\nabout " GENERAL ". Stop.",
              "？？？「" GENERAL_JA "のことを\n　かぎ回っているな。\n　やめておけ。」"),
        RPG_T(HERO ": He burned my\nvillage. I'm going to\nkill him.",
              HERO_JA "「やつはおれの村を\n　焼いた。\n　この手でたおす。」"),
        RPG_T(PARTNER ": Name's " PARTNER ". His\nbounty is mine, farm boy.",
              PARTNER_JA "「おれは" PARTNER_JA "。\n　やつの賞金はおれのものだ、\n　いなか者。」"),
        RPG_T(PARTNER ": Revenge makes you\nslow. Slow gets you\nkilled.",
              PARTNER_JA "「ふくしゅうは\n　足をにぶらせる。\n　にぶいやつから死ぬ。」"),
        RPG_T(HERO ": Stay out of my way.",
              HERO_JA "「じゃまをするな。」"),
        RPG_T(PARTNER ": He camps in the old\nfort, east of here. Try not\nto die before I get paid.",
              PARTNER_JA "「やつは東の古い砦に\n　いる。おれが賞金をもらう\n　まで、死ぬなよ。」"),
        RPG_T(PARTNER " leaves through\nthe east gate.",
              PARTNER_JA "は東の門から\n去っていった。"),
    };

    constexpr bn::string_view east_gate_pages[] = {
        RPG_T("The old highway.\nNothing calls you\nthat way yet.",
              "古い街道だ。\n今はまだ、そちらへ\n行く用はない。"),
    };

    constexpr bn::string_view fort_pages[] = {
        RPG_T(PARTNER " is on one knee.\nA man in red armor\nstands over him.",
              PARTNER_JA "がひざをついている。\n赤いよろいの男が、\nそれを見下ろしていた。"),
        RPG_T(GENERAL ": A bounty hunter\nand a stray. What small\nfoes the Emperor has.",
              GENERAL_JA "「賞金かせぎと\n　のら犬か。炎帝さまの敵も\n　小さくなったものだ。」"),
        RPG_T(HERO ": " GENERAL "!!",
              HERO_JA "「" GENERAL_JA "！！」"),
        RPG_T(PARTNER ": Tch. You again...\nHe's too much for one.",
              PARTNER_JA "「ちっ、またお前か……\n　こいつは一人じゃ\n　手におえない。」"),
        RPG_T(PARTNER ": Fine. Just this\nonce. Keep up, " HERO "!",
              PARTNER_JA "「いいだろう。\n　今回だけだ。\n　おくれるなよ、" HERO_JA "！」"),
        RPG_T(PARTNER " fights at\nyour side!",
              PARTNER_JA "がともに戦う！"),
    };

    constexpr bn::string_view fort_again_pages[] = {
        RPG_T(PARTNER ": Again! And don't\nfall behind this time!",
              PARTNER_JA "「もう一度だ！\n　今度はおくれるなよ！」"),
    };

    constexpr bn::string_view fort_victory_pages[] = {
        RPG_T(GENERAL ": ...Enough. You have\nearned the Emperor's eye.\nPray you earn no more.",
              GENERAL_JA "「……そこまでだ。\n　炎帝さまの目にとまったな。\n　これ以上は望まぬことだ。」"),
        RPG_T(GENERAL " vanishes in a\npillar of flame.",
              GENERAL_JA "は火柱の中に\n消えた。"),
        RPG_T(PARTNER ": He got away.\nSo did my bounty.",
              PARTNER_JA "「にげられたか。\n　おれの賞金もな。」"),
        RPG_T(PARTNER ": ...You fight angry.\nBut you didn't run.",
              PARTNER_JA "「……お前の戦いは\n　いかりまかせだ。\n　だが、にげなかった。」"),
        RPG_T(PARTNER ": I owe you one, and\nI hate owing. I'm coming\nalong until it's repaid.",
              PARTNER_JA "「借りができた。\n　借りはきらいでな。\n　返すまでついていく。」"),
        RPG_T(HERO ": ...Do as you like.",
              HERO_JA "「……好きにしろ。」"),
        RPG_T(PARTNER " joined the party!",
              PARTNER_JA "が仲間になった！"),
        RPG_T("- Chapter 3 -\nThe Star-Reader Girl",
              "- 第3章 -\n星読みの少女"),
    };

    constexpr EncounterGroup fort_battle = { { EnemyId::drake, EnemyId::cult_soldier }, 2 };

    // ---------------------------------------------------------------------------------------------------------------
    // Chapter 3 "The Star-Reader Girl"
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view seer_road_pages[] = {
        "A mountain path climbs\nnorth. Nothing calls you\nthat way yet.",
    };

    constexpr bn::string_view seer_village_pages[] = {
        "A girl looks up from a\nbowl of still water.",
        "???: One of ash, one of\nsteel. The stars said\nyou would come.",
        PARTNER ": She means us?\nI'm not paying for a\nfortune.",
        SEER ": I am " SEER ". I read\nthe stars for " RPG_NAME_SEER_TOWN ".",
        SEER ": The one you hunt went\nnorth, past the " GATE ".\nIt is shut to all.",
        HERO ": Then how do we\ncross it?",
        SEER ": The stars above it\nwent dark. The mirror in\nthe Star Ruins can see.",
        SEER ": Take me there. I\nmust know why my sky\nis going out.",
        SEER " joined the party!",
    };

    constexpr bn::string_view ruins_gate_pages[] = {
        "A stone path climbs to\nold ruins. The villagers\nkeep away from it.",
    };

    constexpr bn::string_view altar_pages[] = {
        "A mirror of black glass\nlies on the altar.\nIt shows no stars.",
        SEER ": Something woke.\nIt guards the mirror!",
    };

    constexpr bn::string_view altar_victory_pages[] = {
        "The guardian crumbles.\nStarlight fills the\nmirror.",
        SEER ": I see the " GATE "...\nand under it, water. An\nold waterway. A way through.",
        SEER ": And I see why the\nstars fade. Ash. It is\nrising to the sky itself.",
        "The mirror cracks. A shard\nshaped like a star stays\nin " SEER "'s hand.",
        "Got the Star Sigil!",
        PARTNER ": A sewer. Great.\nLead on, stargazer.",
        SEER ": The sluice is by the\nlake, in the east of\nthe fields.",
    };

    constexpr bn::string_view sluice_pages[] = {
        "An old sluice gate by\nthe lake. Its lock is\nshaped like a star.",
    };

    constexpr bn::string_view warden_pages[] = {
        "Warden: Halt. None pass\nunder the " GATE ". By order\nof General " GENERAL ".",
        HERO ": Then he is on\nthe other side.",
        "Warden: And you never\nwill be. Drown them!",
    };

    constexpr bn::string_view warden_victory_pages[] = {
        "The warden falls. Behind\nhim, stairs climb toward\ndaylight.",
        SEER ": The " GATE " is\nabove us. We are through.",
        HERO ": " GENERAL ". " RPG_NAME_EMPEROR ".\nI'm coming.",
        SEER ": Your star burns very\nhot, " HERO ". Do not let\nit burn you up.",
        "- Chapter 4 -\nThe Scorched Land",
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Chapter 4 "The Scorched Land"
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view wasteland_pages[] = {
        "Daylight. But no sky:\nonly a gray ceiling\nof smoke.",
        "No fields. No forest.\nAsh, to the horizon.",
        PARTNER ": This isn't a\nbattlefield. Nobody\nfought here.",
        SEER ": I cannot hear the\nland. It is not asleep.\nIt is gone.",
        HERO ": Smoke, to the west.\nChimney smoke.\nSomeone is alive.",
    };

    constexpr bn::string_view chronicler_pages[] = {
        "An old woman marks a\nscorched map with\ncharcoal.",
        "Chronicler: Travelers?\nFrom beyond the " GATE "?\nThen you don't know.",
        "Chronicler: Look. Every\nblack mark is a land\nthat burned.",
        "The map is almost\nentirely black.",
        "Chronicler: Eight parts in\nten of the world. Gone.\nYours is what is left.",
        HERO ": ...Eight in ten.\n" RPG_NAME_VILLAGE " was one spark\nin all of this.",
        PARTNER ": And I was\ncounting bounties.",
        "Chronicler: The Emperor\ndoes not conquer.\nHe feeds a fire.",
        "Chronicler: His general\ncamps past the Searing\nCanyon, to the northeast.",
        SEER ": " HERO ". Is revenge\nstill all you want?",
        HERO ": ...I don't know\nanymore.",
    };

    constexpr bn::string_view canyon_gate_pages[] = {
        "Heat rolls out of a\ncanyon. Better to learn\nwhat lies ahead first.",
    };

    constexpr bn::string_view wyrm_pages[] = {
        "The lava stirs. Something\nvast uncoils from it.",
        SEER ": It was a river\nspirit once. The fire\nate it hollow.",
        PARTNER ": Mourn it later.\nIt's hungry!",
    };

    constexpr bn::string_view wyrm_victory_pages[] = {
        "The wyrm sinks into the\nlava. For a moment, the\nwater in it sings.",
        "From the canyon's rim:\na plain of black glass,\nand a tower of flame.",
        HERO ": That's where\nthey are. All of them.",
        "- Chapter 5 -",
    };

    constexpr bn::string_view beyond_pages[] = {
        "The path goes on toward\nthe tower of flame.\n(To be continued.)",
    };

    constexpr EncounterGroup wyrm_battle = { { EnemyId::ember_wyrm }, 1 };

    constexpr EncounterGroup altar_battle = { { EnemyId::star_guardian }, 1 };
    constexpr EncounterGroup warden_battle = { { EnemyId::warden, EnemyId::cult_soldier }, 2 };

    constexpr int partner_min_level = 4;
    constexpr int seer_min_level = 6;

    /// A story character joins at the hero's level (or `min_level` if that is higher), already armed.
    void join_member(Party& party, ClassId class_id, int min_level, ItemId weapon)
    {
        int level = party.members[0].level;

        if(party.join(class_id, level < min_level ? min_level : level))
        {
            PartyMember& member = party.members[party.size - 1];
            member.weapon = weapon;
            member.restore();
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
            join_member(state.party, ClassId::lancer, partner_min_level, ItemId::iron_spear);
            story_progress = progress::ch2_allied;
        }
        else
        {
            say_pages(ui, fort_again_pages);    // After losing the battle: no need to repeat it all.
        }

        result.battle = &fort_battle;
        break;

    case StoryId::ch3_seer_road:
        say_pages(ui, seer_road_pages);
        result.step_back = true;
        break;

    case StoryId::ch3_seer_village:
        say_pages(ui, seer_village_pages);
        join_member(state.party, ClassId::mage, seer_min_level, ItemId::oak_staff);
        story_progress = progress::ch3_seer;
        break;

    case StoryId::ch3_ruins_gate:
        say_pages(ui, ruins_gate_pages);
        result.step_back = true;
        break;

    case StoryId::ch3_altar:
        say_pages(ui, altar_pages);
        result.battle = &altar_battle;
        break;

    case StoryId::ch3_sluice:
        say_pages(ui, sluice_pages);
        result.step_back = true;
        break;

    case StoryId::ch3_warden:
        say_pages(ui, warden_pages);
        result.battle = &warden_battle;
        break;

    case StoryId::ch4_wasteland:
        say_pages(ui, wasteland_pages);
        story_progress = progress::ch4_wasteland;
        break;

    case StoryId::ch4_chronicler:
        say_pages(ui, chronicler_pages);
        story_progress = progress::ch4_truth;
        break;

    case StoryId::ch4_canyon_gate:
        say_pages(ui, canyon_gate_pages);
        result.step_back = true;
        break;

    case StoryId::ch4_wyrm:
        say_pages(ui, wyrm_pages);
        result.battle = &wyrm_battle;
        break;

    case StoryId::ch4_beyond:
        say_pages(ui, beyond_pages);
        result.step_back = true;
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

    case StoryId::ch3_altar:
        // The key item that opens the sluice of the waterway (the trigger there checks the progress).
        say_pages(ui, altar_victory_pages);
        state.party.add_item(ItemId::star_sigil);
        state.story_progress = progress::ch3_sigil;
        break;

    case StoryId::ch3_warden:
        say_pages(ui, warden_victory_pages);
        state.story_progress = progress::ch4_start;
        break;

    case StoryId::ch4_wyrm:
        say_pages(ui, wyrm_victory_pages);
        state.story_progress = progress::ch5_start;
        break;

    default:
        break;
    }
}

}
