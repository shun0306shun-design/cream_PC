#include "rpg_story.h"

#include "bn_assert.h"

#include "rpg_ui.h"
#include "rpg_events.h"
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
              "兵士「すべて焼きはらえ。\n　焔帝さまのために。」"),
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
              GENERAL_JA "「賞金かせぎと\n　のら犬か。焔帝さまの敵も\n　小さくなったものだ。」"),
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
              GENERAL_JA "「……そこまでだ。\n　焔帝さまの目にとまったな。\n　これ以上は望まぬことだ。」"),
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
        RPG_T("A mountain path climbs\nnorth. Nothing calls you\nthat way yet.",
              "北へ山道がのびている。\n今はまだ、\n行く理由がない。"),
    };

    constexpr bn::string_view seer_village_pages[] = {
        RPG_T("A girl looks up from a\nbowl of still water.",
              "少女が、水をはった鉢から\n顔を上げた。"),
        RPG_T("???: One of ash, one of\nsteel. The stars said\nyou would come.",
              "？？？「灰の者と、鋼の者。\n　星は、あなたたちが\n　来ると告げていました。」"),
        RPG_T(PARTNER ": She means us?\nI'm not paying for a\nfortune.",
              RPG_NAME_PARTNER_JA "「俺たちのことか？\n　占いの代金なら\n　払わないぞ。」"),
        RPG_T(SEER ": I am " SEER ". I read\nthe stars for " RPG_NAME_SEER_TOWN ".",
              RPG_NAME_SEER_JA "「わたしは" RPG_NAME_SEER_JA "。\n　" RPG_NAME_SEER_TOWN_JA "で\n　星を読んでいます。」"),
        RPG_T(SEER ": The one you hunt went\nnorth, past the " GATE ".\nIt is shut to all.",
              RPG_NAME_SEER_JA "「あなたが追う者は北へ、\n　" RPG_NAME_GATE_JA "の向こうへ。\n　門は閉ざされています。」"),
        RPG_T(HERO ": Then how do we\ncross it?",
              RPG_NAME_HERO_JA "「なら、どうやって\n　越えればいい？」"),
        RPG_T(SEER ": The stars above it\nwent dark. The mirror in\nthe Star Ruins can see.",
              RPG_NAME_SEER_JA "「関所の上の星が消えました。\n　星見の遺跡の鏡なら\n　見通せるはずです。」"),
        RPG_T(SEER ": Take me there. I\nmust know why my sky\nis going out.",
              RPG_NAME_SEER_JA "「連れていってください。\n　なぜ空が消えていくのか、\n　知りたいのです。」"),
        RPG_T(SEER " joined the party!",
              RPG_NAME_SEER_JA "が仲間になった！"),
    };

    constexpr bn::string_view ruins_gate_pages[] = {
        RPG_T("A stone path climbs to\nold ruins. The villagers\nkeep away from it.",
              "石の道が古い遺跡へ\n続いている。村人たちは\n近よらないようだ。"),
    };

    constexpr bn::string_view altar_pages[] = {
        RPG_T("A mirror of black glass\nlies on the altar.\nIt shows no stars.",
              "祭壇に黒いガラスの鏡が\n置かれている。\n星はひとつも映らない。"),
        RPG_T(SEER ": Something woke.\nIt guards the mirror!",
              RPG_NAME_SEER_JA "「何かが目覚めました。\n　鏡を守っています！」"),
    };

    constexpr bn::string_view altar_victory_pages[] = {
        RPG_T("The guardian crumbles.\nStarlight fills the\nmirror.",
              "守護者がくずれ落ちた。\n鏡に星の光が満ちていく。"),
        RPG_T(SEER ": I see the " GATE "...\nand under it, water. An\nold waterway. A way through.",
              RPG_NAME_SEER_JA "「" RPG_NAME_GATE_JA "が見えます……\n　その下に、水。古い水路。\n　通り抜けられます。」"),
        RPG_T(SEER ": And I see why the\nstars fade. Ash. It is\nrising to the sky itself.",
              RPG_NAME_SEER_JA "「星が消える理由も\n　見えました。灰です。\n　灰が空までのぼっている。」"),
        RPG_T("The mirror cracks. A shard\nshaped like a star stays\nin " SEER "'s hand.",
              "鏡がひび割れた。\n星の形のかけらが、\n" RPG_NAME_SEER_JA "の手に残った。"),
        RPG_T("Got the Star Sigil!",
              "星の紋章を手に入れた！"),
        RPG_T(PARTNER ": A sewer. Great.\nLead on, stargazer.",
              RPG_NAME_PARTNER_JA "「下水か。最高だな。\n　案内しろよ、星読み。」"),
        RPG_T(SEER ": The sluice is by the\nlake, in the east of\nthe fields.",
              RPG_NAME_SEER_JA "「水門は湖のそば、\n　草原の東にあります。」"),
    };

    constexpr bn::string_view sluice_pages[] = {
        RPG_T("An old sluice gate by\nthe lake. Its lock is\nshaped like a star.",
              "湖のそばに古い水門がある。\n錠は星の形をしている。"),
    };

    constexpr bn::string_view warden_pages[] = {
        RPG_T("Warden: Halt. None pass\nunder the " GATE ". By order\nof General " GENERAL ".",
              "番人「止まれ。" RPG_NAME_GATE_JA "の下は\n　誰も通さん。" RPG_NAME_GENERAL_1_JA "将軍の\n　命令だ。」"),
        RPG_T(HERO ": Then he is on\nthe other side.",
              RPG_NAME_HERO_JA "「なら、奴は\n　この向こうにいるんだな。」"),
        RPG_T("Warden: And you never\nwill be. Drown them!",
              "番人「お前たちは\n　たどり着けん。\n　沈めてしまえ！」"),
    };

    constexpr bn::string_view warden_victory_pages[] = {
        RPG_T("The warden falls. Behind\nhim, stairs climb toward\ndaylight.",
              "番人が倒れた。その奥で、\n階段が日の光へと\n続いている。"),
        RPG_T(SEER ": The " GATE " is\nabove us. We are through.",
              RPG_NAME_SEER_JA "「" RPG_NAME_GATE_JA "は、この真上です。\n　通り抜けました。」"),
        RPG_T(HERO ": " GENERAL ". " RPG_NAME_EMPEROR ".\nI'm coming.",
              RPG_NAME_HERO_JA "「" RPG_NAME_GENERAL_1_JA "。" RPG_NAME_EMPEROR_JA "。\n　今、行くぞ。」"),
        RPG_T(SEER ": Your star burns very\nhot, " HERO ". Do not let\nit burn you up.",
              RPG_NAME_SEER_JA "「あなたの星は、とても\n　熱く燃えています。\n　焼きつくされないで。」"),
        RPG_T("- Chapter 4 -\nThe Scorched Land",
              "- 第4章 -\n焦土の大地"),
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Chapter 4 "The Scorched Land"
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view wasteland_pages[] = {
        RPG_T("Daylight. But no sky:\nonly a gray ceiling\nof smoke.",
              "日の光だ。だが空はない。\nあるのは、灰色の\n煙の天井だけだ。"),
        RPG_T("No fields. No forest.\nAsh, to the horizon.",
              "畑も、森もない。\n地平線まで、灰。"),
        RPG_T(PARTNER ": This isn't a\nbattlefield. Nobody\nfought here.",
              RPG_NAME_PARTNER_JA "「ここは戦場じゃない。\n　誰も戦ってなんか\n　いないんだ。」"),
        RPG_T(SEER ": I cannot hear the\nland. It is not asleep.\nIt is gone.",
              RPG_NAME_SEER_JA "「大地の声が聞こえません。\n　眠っているのではない。\n　もう、いないのです。」"),
        RPG_T(HERO ": Smoke, to the west.\nChimney smoke.\nSomeone is alive.",
              RPG_NAME_HERO_JA "「西に煙が見える。\n　かまどの煙だ。\n　誰かが生きている。」"),
    };

    constexpr bn::string_view chronicler_pages[] = {
        RPG_T("An old woman marks a\nscorched map with\ncharcoal.",
              "老婆が、焼けこげた地図に\n木炭で印をつけている。"),
        RPG_T("Chronicler: Travelers?\nFrom beyond the " GATE "?\nThen you don't know.",
              "記録者「旅の人かい？\n　" RPG_NAME_GATE_JA "の向こうから？\n　なら、知らないんだね。」"),
        RPG_T("Chronicler: Look. Every\nblack mark is a land\nthat burned.",
              "記録者「ごらん。\n　黒い印はすべて、\n　燃えた土地だよ。」"),
        RPG_T("The map is almost\nentirely black.",
              "地図は、ほとんど\nまっ黒だった。"),
        RPG_T("Chronicler: Eight parts in\nten of the world. Gone.\nYours is what is left.",
              "記録者「世界の八割。\n　もう無い。あんたたちの\n　国が、残りさ。」"),
        RPG_T(HERO ": ...Eight in ten.\n" RPG_NAME_VILLAGE " was one spark\nin all of this.",
              RPG_NAME_HERO_JA "「……八割。\n　" RPG_NAME_VILLAGE_JA "村は、この中の\n　火の粉ひとつだったのか。」"),
        RPG_T(PARTNER ": And I was\ncounting bounties.",
              RPG_NAME_PARTNER_JA "「俺は、賞金なんか\n　数えていたのか。」"),
        RPG_T("Chronicler: The Emperor\ndoes not conquer.\nHe feeds a fire.",
              "記録者「焔帝は征服しない。\n　火に、まきを\n　くべているだけさ。」"),
        RPG_T("Chronicler: His general\ncamps past the Searing\nCanyon, to the northeast.",
              "記録者「奴の将軍は\n　北東、焦熱の峡谷の\n　先に陣を張っている。」"),
        RPG_T(SEER ": " HERO ". Is revenge\nstill all you want?",
              RPG_NAME_SEER_JA "「" RPG_NAME_HERO_JA "。今でも望みは、\n　復讐だけですか？」"),
        RPG_T(HERO ": ...I don't know\nanymore.",
              RPG_NAME_HERO_JA "「……もう、\n　分からない。」"),
    };

    constexpr bn::string_view canyon_gate_pages[] = {
        RPG_T("Heat rolls out of a\ncanyon. Better to learn\nwhat lies ahead first.",
              "峡谷から熱気が\n吹きつけてくる。先に、\nこの先のことを知るべきだ。"),
    };

    constexpr bn::string_view wyrm_pages[] = {
        RPG_T("The lava stirs. Something\nvast uncoils from it.",
              "溶岩がうごめく。\n巨大な何かが、\n身をほどいていく。"),
        RPG_T(SEER ": It was a river\nspirit once. The fire\nate it hollow.",
              RPG_NAME_SEER_JA "「かつては川の精霊でした。\n　炎に、中身を\n　食いつくされたのです。」"),
        RPG_T(PARTNER ": Mourn it later.\nIt's hungry!",
              RPG_NAME_PARTNER_JA "「なげくのは後だ。\n　腹をすかせてるぞ！」"),
    };

    constexpr bn::string_view wyrm_victory_pages[] = {
        RPG_T("The wyrm sinks into the\nlava. For a moment, the\nwater in it sings.",
              "ワームが溶岩に沈んでいく。\n一瞬、その中の水が\n歌ったように聞こえた。"),
        RPG_T("From the canyon's rim:\na plain of black glass,\nand a tower of flame.",
              "峡谷のふちから見えたのは、\n黒いガラスの平原と、\n炎の塔だった。"),
        RPG_T(HERO ": That's where\nthey are. All of them.",
              RPG_NAME_HERO_JA "「あそこに奴らがいる。\n　全員が。」"),
        RPG_T("- Chapter 5 -\nAshes of Betrayal",
              "- 第5章 -\n裏切りの灰"),
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Chapter 5 "Ashes of Betrayal"
    // ---------------------------------------------------------------------------------------------------------------
    #define NOX RPG_NAME_DEFECTOR

    constexpr bn::string_view dorn_arrival_pages[] = {
        RPG_T(RPG_NAME_MINE_TOWN ". The mine works.\nCarts of ore roll out\nunder the cult's banner.",
              "鉱山町" RPG_NAME_MINE_TOWN_JA "。\n教団の旗の下、鉱石の\n荷車が出ていく。"),
        RPG_T("Miner: We dig, they let us\nlive. Don't ask us for\nhelp, traveler.",
              "鉱夫「掘れば、生かして\n　もらえる。おれたちに\n　助けを求めるな。」"),
        RPG_T(RPG_NAME_PARTNER ": A town that kneels.\n...It's still standing,\nat least.",
              RPG_NAME_PARTNER_JA "「ひざまずいた町か。\n　……まだ立っているだけ\n　ましだな。」"),
    };

    constexpr bn::string_view deserter_pages[] = {
        RPG_T("Behind the tavern, cult\nsoldiers corner a man\nin a torn robe.",
              "酒場の裏で、教団兵が\nやぶれたローブの男を\n追いつめている。"),
        RPG_T("Soldier: Deserter. The\ngeneral wants you back\nin pieces.",
              "教団兵「脱走兵め。\n　将軍は、お前をばらばらで\n　連れ戻せとおおせだ。」"),
        RPG_T(RPG_NAME_HERO ": Cult soldiers.\nThat's all I need\nto know.",
              RPG_NAME_HERO_JA "「教団兵か。\n　それだけ分かれば\n　じゅうぶんだ。」"),
    };

    constexpr bn::string_view deserter_victory_pages[] = {
        RPG_T(RPG_NAME_DEFECTOR ": ...Why help me? I\nwore that robe too.\nI am " RPG_NAME_DEFECTOR ".",
              RPG_NAME_DEFECTOR_JA "「……なぜ助ける。\n　私もあのローブを着ていた。\n　" RPG_NAME_DEFECTOR_JA "という。」"),
        RPG_T(RPG_NAME_DEFECTOR ": General " RPG_NAME_GENERAL_3 " sits\nin the Ash Cathedral.\nI know a way in.",
              RPG_NAME_DEFECTOR_JA "「灰将" RPG_NAME_GENERAL_3_JA "は\n　灰の聖堂にいる。\n　抜け道を知っている。」"),
        RPG_T(RPG_NAME_DEFECTOR ": Through the old\nmine, north of town.\nLet me guide you.",
              RPG_NAME_DEFECTOR_JA "「町の北の廃坑を\n　抜けるのだ。\n　案内させてくれ。」"),
        RPG_T(RPG_NAME_DEFECTOR " comes along!",
              RPG_NAME_DEFECTOR_JA "が同行する！"),
    };

    constexpr bn::string_view mine_gate_pages[] = {
        RPG_T("The old mine. The shaft\nis boarded up.",
              "古い廃坑だ。\n入口は板でふさがれている。"),
    };

    constexpr bn::string_view overseer_pages[] = {
        RPG_T("Overseer: " RPG_NAME_DEFECTOR ". " RPG_NAME_GENERAL_3 "\nmisses her favorite\nfire-starter.",
              "監督官「" RPG_NAME_DEFECTOR_JA "。\n　" RPG_NAME_GENERAL_3_JA "さまが、お気に入りの\n　火つけ役をお待ちだ。」"),
        RPG_T(RPG_NAME_DEFECTOR ": I am done\nlighting fires.",
              RPG_NAME_DEFECTOR_JA "「火をつけるのは、\n　もうやめた。」"),
    };

    constexpr bn::string_view overseer_victory_pages[] = {
        RPG_T(RPG_NAME_DEFECTOR ": Beyond this shaft\nlies a ruin, and past\nit, the cathedral.",
              RPG_NAME_DEFECTOR_JA "「この坑道の先に\n　廃墟がある。その向こうが\n　聖堂だ。」"),
        RPG_T(RPG_NAME_SEER ": You fight like one\nlooking for a place\nto die.",
              RPG_NAME_SEER_JA "「あなたは、死に場所を\n　さがすように戦うのですね。」"),
        RPG_T(RPG_NAME_DEFECTOR ": ...Perhaps. Until\nthen, use me.",
              RPG_NAME_DEFECTOR_JA "「……かもしれん。\n　それまで、使ってくれ。」"),
        RPG_T(RPG_NAME_DEFECTOR " joined the party!",
              RPG_NAME_DEFECTOR_JA "が仲間になった！"),
    };

    constexpr bn::string_view kazami_arrival_pages[] = {
        RPG_T(RPG_NAME_PARTNER " stops walking.",
              RPG_NAME_PARTNER_JA "が足を止めた。"),
        RPG_T(RPG_NAME_PARTNER ": ...That wall.\nThat was my house.",
              RPG_NAME_PARTNER_JA "「……あの壁。\n　あれが、おれの家だ。」"),
        RPG_T(RPG_NAME_HERO ": " RPG_NAME_PARTNER_HOME ". You are\nfrom here.",
              RPG_NAME_HERO_JA "「" RPG_NAME_PARTNER_HOME_JA "村。\n　ここが、お前の……」"),
        RPG_T(RPG_NAME_PARTNER ": Burned. Like yours.\nI never said. Saying\nchanges nothing.",
              RPG_NAME_PARTNER_JA "「焼かれた。お前の村と\n　同じだ。言わなかった。\n　言っても変わらん。」"),
    };

    constexpr bn::string_view confession_pages[] = {
        RPG_T("A grave marker, tended\nonce, long ago.",
              "墓標がある。\nずっと昔に、だれかが\n手入れしていたようだ。"),
        RPG_T(RPG_NAME_DEFECTOR ": ...I know this\nplace. I was in the unit\nthat burned this village.",
              RPG_NAME_DEFECTOR_JA "「……ここを知っている。\n　この村を焼いた隊に、\n　私はいた。」"),
        RPG_T(RPG_NAME_PARTNER "'s spear is at\n" RPG_NAME_DEFECTOR "'s throat.",
              RPG_NAME_PARTNER_JA "の槍が、\n" RPG_NAME_DEFECTOR_JA "ののどに向けられた。"),
    };

    constexpr bn::string_view confession_stop_pages[] = {
        RPG_T(RPG_NAME_HERO ": " RPG_NAME_PARTNER ". Not like this.",
              RPG_NAME_HERO_JA "「" RPG_NAME_PARTNER_JA "。\n　こんなやり方はよせ。」"),
        RPG_T(RPG_NAME_PARTNER ": ...You'd shield\nhim. Fine.",
              RPG_NAME_PARTNER_JA "「……かばうのか。\n　好きにしろ。」"),
    };

    constexpr bn::string_view confession_watch_pages[] = {
        RPG_T("The spear trembles.\nIt does not strike.",
              "槍先がふるえる。\nだが、突かれはしなかった。"),
        RPG_T(RPG_NAME_DEFECTOR ": Do it. I came\nhere for that.",
              RPG_NAME_DEFECTOR_JA "「やれ。\n　そのために来た。」"),
    };

    constexpr bn::string_view confession_leave_pages[] = {
        RPG_T(RPG_NAME_PARTNER ": I can't walk\nbeside him.",
              RPG_NAME_PARTNER_JA "「こいつとは、\n　ならんで歩けん。」"),
        RPG_T(RPG_NAME_PARTNER " left the party.",
              RPG_NAME_PARTNER_JA "がパーティを離れた。"),
    };

    constexpr bn::string_view cathedral_gate_pages[] = {
        RPG_T("A path climbs north,\ntoward a black spire.",
              "北へ道がのびている。\n黒い尖塔が見える。"),
    };

    constexpr bn::string_view east_gate_ch5_pages[] = {
        RPG_T("The cathedral road. Its\ngate is shut to all but\nthe faithful.",
              "聖堂への道だ。門は\n信徒のほかには\n閉ざされている。"),
    };

    constexpr bn::string_view zelma_pages[] = {
        RPG_T(RPG_NAME_GENERAL_3 ": " RPG_NAME_DEFECTOR ". My lost lamb.\nYou brought me guests.",
              RPG_NAME_GENERAL_3_JA "「" RPG_NAME_DEFECTOR_JA "。迷える子羊。\n　お客を連れてきたのね。」"),
        RPG_T(RPG_NAME_GENERAL_3 ": " RPG_NAME_PARTNER_HOME "? I gave that\norder. It burned\nbeautifully.",
              RPG_NAME_GENERAL_3_JA "「" RPG_NAME_PARTNER_HOME_JA "？ わたしが\n　命じたわ。美しく\n　燃えたでしょう。」"),
        RPG_T(RPG_NAME_DEFECTOR ": The Emperor's dream\nis a lie, " RPG_NAME_GENERAL_3 ".",
              RPG_NAME_DEFECTOR_JA "「焔帝の夢は\n　いつわりだ、" RPG_NAME_GENERAL_3_JA "。」"),
    };

    constexpr bn::string_view zelma_again_pages[] = {
        RPG_T(RPG_NAME_GENERAL_3 ": Back for more ash?",
              RPG_NAME_GENERAL_3_JA "「また灰に\n　なりに来たの？」"),
    };

    constexpr bn::string_view zelma_reinforce_pages[] = {
        RPG_T("A spear flies in from\nthe doors. " RPG_NAME_PARTNER "\nis back!",
              "扉から槍が飛んできた。\n" RPG_NAME_PARTNER_JA "が戻ってきた！"),
    };

    constexpr bn::string_view zelma_victory_pages[] = {
        RPG_T(RPG_NAME_GENERAL_3 ": Enough. The capital\nburns next. Come\nand watch.",
              RPG_NAME_GENERAL_3_JA "「もういいわ。\n　次は王都が燃える。\n　見にいらっしゃい。」"),
        RPG_T(RPG_NAME_GENERAL_3 " vanishes in the\nsmoke of the pyre.",
              RPG_NAME_GENERAL_3_JA "は、火葬台の煙の\n中へ消えた。"),
        RPG_T(RPG_NAME_PARTNER ": " RPG_NAME_DEFECTOR ". I don't\nforgive you. But\ndon't die.",
              RPG_NAME_PARTNER_JA "「" RPG_NAME_DEFECTOR_JA "。許さん。\n　だが、死ぬな。」"),
        RPG_T(RPG_NAME_PARTNER ": Atone alive.",
              RPG_NAME_PARTNER_JA "「償いは、\n　生きてやれ。」"),
    };

    constexpr bn::string_view zelma_stopped_pages[] = {
        RPG_T(RPG_NAME_DEFECTOR ": ...You stood\nbetween us, " RPG_NAME_HERO ".\nI will not waste it.",
              RPG_NAME_DEFECTOR_JA "「……あの時、間に\n　立ってくれたな、" RPG_NAME_HERO_JA "。\n　むだにはしない。」"),
    };

    constexpr bn::string_view zelma_watched_pages[] = {
        RPG_T(RPG_NAME_DEFECTOR ": ...Then I will\nlive. That is harder.",
              RPG_NAME_DEFECTOR_JA "「……ならば生きよう。\n　そのほうが、つらい。」"),
    };

    constexpr bn::string_view zelma_end_pages[] = {
        RPG_T(RPG_NAME_HERO ": The capital.\n" RPG_NAME_CAPITAL ". We go.",
              RPG_NAME_HERO_JA "「王都" RPG_NAME_CAPITAL_JA "。\n　行くぞ。」"),
        RPG_T("- Chapter 6 -\nThe Sinking Capital",
              "- 第6章 -\n沈む王都"),
    };


    // ---------------------------------------------------------------------------------------------------------------
    // Chapter 6 "The Sinking Capital"
    // ---------------------------------------------------------------------------------------------------------------
    #define CAPITAL RPG_NAME_CAPITAL
    #define GENERAL2 RPG_NAME_GENERAL_2

    constexpr bn::string_view audience_pages[] = {
        RPG_T("Guards lead you through\na city under siege,\nto the king.",
              "衛兵に連れられ、\n籠城の都を抜けて\n王の前へ通された。"),
        RPG_T("King: You drove a general\nfrom her altar. " RPG_NAME_CAPITAL "\nneeds such blades.",
              "王「灰将を祭壇から\n　追いはらったそうだな。\n　その剣を貸してほしい。」"),
        RPG_T(RPG_NAME_HERO ": A general is\ncoming here?",
              RPG_NAME_HERO_JA "「灰将が、\n　ここに来るのか。」"),
        RPG_T("King: " RPG_NAME_GENERAL_2 ", at dawn.\nStand at our gate. Rest\ntonight, at the inn.",
              "王「夜明けに、" RPG_NAME_GENERAL_2_JA "が。\n　城門に立ってくれ。\n　今夜は宿で休むがよい。」"),
    };

    constexpr bn::string_view inn_before_pages[] = {
        RPG_T("Innkeeper: No rooms.\nThe whole city sleeps\nin its boots.",
              "宿屋「部屋はないよ。\n　都じゅうが、くつを\n　はいたまま寝てるんだ。」"),
    };

    constexpr bn::string_view inn_rest_pages[] = {
        RPG_T("Innkeeper: The king pays\ntonight. Sleep while\nyou can.",
              "宿屋「今夜のお代は王さま\n　もちだ。眠れるうちに\n　眠っておきな。」"),
    };

    constexpr bn::string_view eve_pages[] = {
        RPG_T("Night. No one sleeps.\nThe others are out\nin the city.",
              "夜。だれも眠れない。\n仲間たちは町に出ている。"),
    };

    constexpr bn::string_view eve_partner_pages[] = {
        RPG_T(RPG_NAME_PARTNER ": Don't chase the\ngeneral tomorrow.\nHold the gate.",
              RPG_NAME_PARTNER_JA "「あした、灰将を\n　追うなよ。\n　門を守れ。」"),
    };

    constexpr bn::string_view eve_seer_pages[] = {
        RPG_T(RPG_NAME_SEER ": I have seen this\ncity burn, in dreams.\nI want to be wrong.",
              RPG_NAME_SEER_JA "「この都が燃えるのを\n　夢で見ました。\n　外れてほしいのです。」"),
    };

    constexpr bn::string_view eve_defector_pages[] = {
        RPG_T(RPG_NAME_DEFECTOR ": " RPG_NAME_GENERAL_2 " is no zealot.\nHe is a soldier.\nThat is worse.",
              RPG_NAME_DEFECTOR_JA "「" RPG_NAME_GENERAL_2_JA "は信者では\n　ない。武人だ。\n　そのほうが悪い。」"),
    };

    constexpr bn::string_view inn_sleep_pages[] = {
        RPG_T("Innkeeper: Sleep until\ndawn?",
              "宿屋「夜明けまで\n　眠るかい？」"),
    };

    constexpr bn::string_view gate_1_pages[] = {
        RPG_T("Dawn. Horns. The enemy\nis at the gate.",
              "夜明け。角笛が鳴る。\n敵が城門にせまる。"),
    };

    constexpr bn::string_view gate_2_pages[] = {
        RPG_T("The second wave climbs\nover the dead.",
              "第二波が、しかばねを\nこえてくる。"),
    };

    constexpr bn::string_view gate_3_pages[] = {
        RPG_T("A third wave. No time\nto breathe.",
              "第三波だ。\n息をつくひまもない。"),
    };

    constexpr bn::string_view gate_held_pages[] = {
        RPG_T("The gate holds. But smoke\nrises from the palace\nbehind you.",
              "城門は守りきった。\nだが、後ろの王城から\n煙が上がっている。"),
        RPG_T(RPG_NAME_HERO ": The palace!",
              RPG_NAME_HERO_JA "「王城だ！」"),
    };

    constexpr bn::string_view palace_closed_pages[] = {
        RPG_T("The palace doors\nare shut.",
              "王城の扉は\n閉ざされている。"),
    };

    constexpr bn::string_view great_gate_pages[] = {
        RPG_T("The great gate is barred\nfor the siege.",
              "大門は、籠城のために\n閉ざされている。"),
    };

    constexpr bn::string_view bardos_pages[] = {
        RPG_T("The king lies wounded\nbefore his throne. A\ngiant in iron turns.",
              "王が玉座の前でたおれて\nいる。鉄のよろいの\n大男がふり向いた。"),
        RPG_T(RPG_NAME_GENERAL_2 ": The gate was bait.\nGood soldiers. You\nheld it well.",
              RPG_NAME_GENERAL_2_JA "「城門はおとりだ。\n　よい兵だな。\n　よく守った。」"),
        RPG_T(RPG_NAME_HERO ": Get away\nfrom him!",
              RPG_NAME_HERO_JA "「王から\n　はなれろ！」"),
    };

    constexpr bn::string_view bardos_after_pages[] = {
        RPG_T(RPG_NAME_GENERAL_2 ": Enough. I have\nwhat I came for.",
              RPG_NAME_GENERAL_2_JA "「ここまでだ。\n　目的は果たした。」"),
        RPG_T("The king is struck down.",
              "王が、斬られた。"),
        RPG_T("King: ...My people.\nI leave them... to you.",
              "王「……民を……\n　民を、たのむ……」"),
        RPG_T(RPG_NAME_HERO ": " RPG_NAME_GENERAL_2 "!!",
              RPG_NAME_HERO_JA "「" RPG_NAME_GENERAL_2_JA "！！」"),
        RPG_T(RPG_NAME_HERO " runs at him, alone.\n" RPG_NAME_PARTNER "'s fist stops\nhim cold.",
              RPG_NAME_HERO_JA "は一人で\n斬りかかろうとした。\n" RPG_NAME_PARTNER_JA "のこぶしが止めた。"),
        RPG_T(RPG_NAME_PARTNER ": You'll die for\nnothing, you fool!",
              RPG_NAME_PARTNER_JA "「むだ死にする気か！\n　この大ばか野郎！」"),
        RPG_T("The ceiling gives way.\nFire and stone fall\nbetween you all.",
              "天井がくずれ落ちる。\n炎と石が、みんなの\n間をへだてた。"),
    };

    constexpr bn::string_view road_lost_pages[] = {
        RPG_T("The pilgrim road is choked\nwith smoke. There is no\ngoing back.",
              "巡礼路は煙に\nふさがれている。\nもう戻れない。"),
    };


    // ---------------------------------------------------------------------------------------------------------------
    // Chapter 7 "The Lamplight"
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view awake_pages[] = {
        RPG_T("- Chapter 7 -\nThe Lamplight",
              "- 第7章 -\n灯火"),
        RPG_T("Firelight. A roof of\nbranches. You are alive.",
              "火のあかり。木の枝の屋根。\n……生きている。"),
        RPG_T("Rico: You're awake! I\npulled you out of the\nriver. You're heavy.",
              "リコ「起きた！ 川から\n　引っぱり上げたんだ。\n　重かったよ。」"),
        RPG_T(RPG_NAME_HERO ": Rico... from " RPG_NAME_ASH_TOWN ".\nThe others? Where\nare they?",
              RPG_NAME_HERO_JA "「リコ……" RPG_NAME_ASH_TOWN_JA "の。\n　みんなは。\n　ほかの三人は？」"),
        RPG_T("Rico: Only you. This is\n" RPG_NAME_REFUGE ". Everyone here\nlost somebody.",
              "リコ「兄ちゃんだけだよ。\n　ここは" RPG_NAME_REFUGE_JA "。みんな、\n　だれかをなくした人だ。」"),
    };

    constexpr bn::string_view awake_charm_pages[] = {
        RPG_T("Rico: Take this. Mum made\nit. For people who\ncome back.",
              "リコ「これ、あげる。\n　母ちゃんが作ったんだ。\n　帰ってきた人のお守り。」"),
        RPG_T("Got Rico's Charm!",
              "リコのお守りを手に入れた！"),
    };

    constexpr bn::string_view rico_pages[] = {
        RPG_T("Rico: The lady who heals\nis by the big house.\nShe never sleeps.",
              "リコ「手当てのお姉ちゃんは\n　大きな家のそばだよ。\n　ぜんぜん寝ないんだ。」"),
    };

    constexpr bn::string_view rico_later_pages[] = {
        RPG_T("Rico: I'm not scared.\nYou came back, so\nthey will too.",
              "リコ「こわくないよ。\n　兄ちゃんが戻ったんだ。\n　みんなも戻るよ。」"),
    };

    constexpr bn::string_view seer_rejoin_pages[] = {
        RPG_T(RPG_NAME_SEER ": " RPG_NAME_HERO "! ...You are\nalive. I looked for your\nstar every night.",
              RPG_NAME_SEER_JA "「" RPG_NAME_HERO_JA "！ ……生きて\n　いたのですね。毎晩、\n　あなたの星をさがしました。」"),
        RPG_T(RPG_NAME_SEER ": I was tending the\nwounded. But I go\nwhere you go.",
              RPG_NAME_SEER_JA "「けが人の手当てを\n　していました。でも、\n　あなたと行きます。」"),
        RPG_T(RPG_NAME_SEER " rejoined the party!",
              RPG_NAME_SEER_JA "がパーティに戻った！"),
    };

    constexpr bn::string_view east_gate_ch7_pages[] = {
        RPG_T("The road east. Not\nalone. Not again.",
              "東への道だ。\n一人では行かない。\nもう二度と。"),
    };

    constexpr bn::string_view defector_rejoin_pages[] = {
        RPG_T("At the broken bridge, a\nman in a torn robe\ncarries the wounded.",
              "こわれた橋で、やぶれた\nローブの男が、けが人を\n運んでいる。"),
        RPG_T(RPG_NAME_DEFECTOR ": " RPG_NAME_HERO ". I meant to\ndie in that castle. Then\nI heard a child cry.",
              RPG_NAME_DEFECTOR_JA "「" RPG_NAME_HERO_JA "か。あの城で\n　死ぬつもりだった。だが、\n　子どもの泣き声がした。」"),
        RPG_T(RPG_NAME_DEFECTOR ": I am still atoning.\nLet me do it beside you.",
              RPG_NAME_DEFECTOR_JA "「償いは、まだ途中だ。\n　となりでやらせてくれ。」"),
        RPG_T(RPG_NAME_DEFECTOR " rejoined the party!",
              RPG_NAME_DEFECTOR_JA "がパーティに戻った！"),
    };

    constexpr bn::string_view partner_rejoin_pages[] = {
        RPG_T(RPG_NAME_PARTNER " crouches by the\nmarsh, watching the fort.",
              RPG_NAME_PARTNER_JA "が沼のほとりで、\n砦を見はっている。"),
        RPG_T(RPG_NAME_PARTNER ": ...You're late.\n" RPG_NAME_GENERAL_2 " is inside. I was\ngoing in alone.",
              RPG_NAME_PARTNER_JA "「……おそいぞ。\n　中に" RPG_NAME_GENERAL_2_JA "がいる。\n　一人で行くところだった。」"),
        RPG_T(RPG_NAME_SEER ": Alone. As someone\ndid, in the castle?",
              RPG_NAME_SEER_JA "「一人で、ですか。\n　城での、だれかのように？」"),
        RPG_T(RPG_NAME_PARTNER ": ...Tch. Fine.\nNot tonight. Back to\nthe fire.",
              RPG_NAME_PARTNER_JA "「……ちっ。分かった。\n　今夜はやめだ。\n　火のところへ戻るぞ。」"),
        RPG_T(RPG_NAME_PARTNER " rejoined the party!",
              RPG_NAME_PARTNER_JA "がパーティに戻った！"),
    };

    constexpr bn::string_view fort_gate_pages[] = {
        RPG_T("The gate of the fort.\nNot yet. Not like\nlast time.",
              "砦の門だ。まだだ。\n前と同じでは、だめだ。"),
    };

    constexpr bn::string_view campfire_pages[] = {
        RPG_T("The four sit by the fire.\n" RPG_NAME_PARTNER " sits apart,\nas always.",
              "四人は火をかこんだ。\n" RPG_NAME_PARTNER_JA "だけは、いつものように\nはなれてすわっている。"),
        RPG_T(RPG_NAME_HERO ": I ran at " RPG_NAME_GENERAL_2 "\nalone. The king died.\nYou were all scattered.",
              RPG_NAME_HERO_JA "「おれは一人で" RPG_NAME_GENERAL_2_JA "に\n　斬りかかった。王は死に、\n　みんなはちりぢりになった。」"),
        RPG_T(RPG_NAME_HERO ": I thought revenge\nwas all I had left.",
              RPG_NAME_HERO_JA "「復讐だけが、おれに\n　残されたものだと\n　思っていた。」"),
        RPG_T(RPG_NAME_HERO ": Then a child pulled\nme out of a river.",
              RPG_NAME_HERO_JA "「でも、子どもが\n　おれを川から\n　引き上げてくれた。」"),
        RPG_T(RPG_NAME_HERO ": I won't fight for\nrevenge. I don't want to\nlose anyone. Not again.",
              RPG_NAME_HERO_JA "「復讐のためじゃない。\n　もう、だれも\n　失くしたくない。」"),
        RPG_T(RPG_NAME_SEER ": ...That is the star\nI was looking for.",
              RPG_NAME_SEER_JA "「……それが、わたしの\n　さがしていた星です。」"),
        RPG_T(RPG_NAME_DEFECTOR ": Then I will guard\nthat wish. It is worth\nmore than my life.",
              RPG_NAME_DEFECTOR_JA "「ならば、その願いを\n　守ろう。私の命より\n　ねうちがある。」"),
        RPG_T(RPG_NAME_PARTNER " gets up, and sits\ndown by the fire.",
              RPG_NAME_PARTNER_JA "が立ち上がり、\n火のそばにすわった。"),
        RPG_T(RPG_NAME_PARTNER ": ..." RPG_NAME_HERO ".",
              RPG_NAME_PARTNER_JA "「……" RPG_NAME_HERO_JA "。」"),
        RPG_T(RPG_NAME_PARTNER ": Next time you run,\nI run with you.\nThat's all.",
              RPG_NAME_PARTNER_JA "「次に走るときは、\n　おれも走る。\n　それだけだ。」"),
    };

    constexpr bn::string_view campfire_before_pages[] = {
        RPG_T("A campfire. People warm\ntheir hands, and say\nlittle.",
              "焚き火だ。人びとが\n手をあたため、\nだまって火を見ている。"),
    };

    constexpr bn::string_view campfire_after_pages[] = {
        RPG_T("The fire burns on.",
              "火は、燃えつづけている。"),
    };

    constexpr bn::string_view keeper_pages[] = {
        RPG_T("Elder: " RPG_NAME_REFUGE " is a small\nlight. Small lights are\nhard to put out.",
              "長老「" RPG_NAME_REFUGE_JA "は小さな\n　灯火だ。小さな火ほど、\n　消えにくいものさ。」"),
    };

    constexpr bn::string_view bardos_last_pages[] = {
        RPG_T(RPG_NAME_GENERAL_2 ": The boy from the\nthrone room. You came\nback with a wall.",
              RPG_NAME_GENERAL_2_JA "「玉座の間の小僧か。\n　壁を連れて\n　戻ってきたな。」"),
        RPG_T(RPG_NAME_HERO ": Not for the king.\nNot for revenge. For\nthe ones still alive.",
              RPG_NAME_HERO_JA "「王のためでも、\n　復讐のためでもない。\n　生きている者のためだ。」"),
        RPG_T(RPG_NAME_GENERAL_2 ": Good. Then this\nwill be a battle.",
              RPG_NAME_GENERAL_2_JA "「よい。ならば、\n　これは戦になる。」"),
    };

    constexpr bn::string_view bardos_last_victory_pages[] = {
        RPG_T(RPG_NAME_GENERAL_2 ": ...A fine wall.\nI have no regrets.",
              RPG_NAME_GENERAL_2_JA "「……よい壁だ。\n　悔いはない。」"),
        RPG_T(RPG_NAME_GENERAL_2 " falls. The first\nAsh General to die.",
              RPG_NAME_GENERAL_2_JA "がたおれた。\n初めて、灰将を討った。"),
        RPG_T(RPG_NAME_PARTNER ": It can be done.\n...It can be done.",
              RPG_NAME_PARTNER_JA "「やれるんだ。\n　……やれるんだな。」"),
    };

    constexpr bn::string_view signal_pages[] = {
        RPG_T("Elder: A general is dead.\nLet the others know\nthey are not alone.",
              "長老「灰将が討たれた。\n　ほかの者にも、\n　一人ではないと知らせよう。」"),
        RPG_T("Signal fires rise from\n" RPG_NAME_REFUGE ", hill after hill.",
              RPG_NAME_REFUGE_JA "から狼煙が上がる。\n丘から丘へ、次々に。"),
        RPG_T("- Chapter 8 -\nCounterattack",
              "- 第8章 -\n反攻"),
    };

    constexpr bn::string_view south_gate_pages[] = {
        RPG_T("A track south, into the\nhills. No reason to\ntake it yet.",
              "南の丘へ続く小道だ。\n今はまだ、\n行く理由がない。"),
    };

    constexpr bn::string_view keeper_after_pages[] = {
        RPG_T("Keeper: A lamp is small.\nThat is why it takes\nso many of us.",
              "灯守り「灯火は小さい。\n　だから、こんなに大勢が\n　要るのさ。」"),
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Chapter 8 "Counterattack"
    // ---------------------------------------------------------------------------------------------------------------
    #define REFUGE RPG_NAME_REFUGE
    #define SHIP RPG_NAME_SHIP
    #define GENERAL3 RPG_NAME_GENERAL_3
    #define GENERAL4 RPG_NAME_GENERAL_4

    constexpr bn::string_view goals_pages[] = {
        RPG_T("Elder: The fires were\nseen. Survivors gather.\nNow we need three things.",
              "長老「狼煙はとどいた。\n　生き残りが集まっている。\n　必要なものは三つだ。」"),
        RPG_T("Elder: A ship, from " RPG_NAME_HARBOR ".\nIts compass lies in the\nSunken Lighthouse.",
              "長老「一つ、" RPG_NAME_HARBOR_JA "の船。\n　羅針盤は、沈んだ灯台の\n　中にある。」"),
        RPG_T("Elder: " RPG_NAME_GENERAL_4 ", the general\nof the north, in the Tower\nof the Ice Wall.",
              "長老「二つ、北の灰将" RPG_NAME_GENERAL_4_JA "。\n　氷壁の塔にいる。」"),
        RPG_T("Elder: And " RPG_NAME_GENERAL_1 ", in the\nVolcano Fort.",
              "長老「三つ、火山の砦の\n　灰将" RPG_NAME_GENERAL_1_JA "だ。」"),
        RPG_T(RPG_NAME_HERO ": ..." RPG_NAME_GENERAL_1 ".",
              RPG_NAME_HERO_JA "「……" RPG_NAME_GENERAL_1_JA "。」"),
    };

    constexpr bn::string_view goals_again_pages[] = {
        RPG_T("Elder: A ship, the north,\nthe volcano. In the\norder you can manage.",
              "長老「船、北、火山。\n　できる順でよい。」"),
    };

    constexpr bn::string_view lighthouse_gate_pages[] = {
        RPG_T("A causeway leads out to\na drowned lighthouse.",
              "沈んだ灯台へ、\n土手道がのびている。"),
    };

    constexpr bn::string_view shipwright_pages[] = {
        RPG_T("Shipwright: The " RPG_NAME_SHIP " floats.\nBut without a compass\nshe goes nowhere.",
              "船大工「" RPG_NAME_SHIP_JA "は浮く。\n　だが羅針盤がなければ、\n　どこへも行けん。」"),
    };

    constexpr bn::string_view shipwright_ready_pages[] = {
        RPG_T("Shipwright: She's ready.\nBoard her from the pier.",
              "船大工「準備はできた。\n　桟橋から乗りな。」"),
    };

    constexpr bn::string_view pier_pages[] = {
        RPG_T("The " RPG_NAME_SHIP " is moored here.\nShe cannot sail without\na compass.",
              RPG_NAME_SHIP_JA "がつながれている。\n羅針盤がなければ\n出航できない。"),
    };

    constexpr bn::string_view pier_ask_pages[] = {
        RPG_T("Where does the " RPG_NAME_SHIP "\nsail?",
              RPG_NAME_SHIP_JA "は、\nどこへ向かう？"),
    };

    constexpr bn::string_view keeper_boss_pages[] = {
        RPG_T("Something coils around\nthe great lamp. It has\nkept it dark for years.",
              "大きな灯に、何かが\nからみついている。\n長いあいだ、光をふさいで。"),
    };

    constexpr bn::string_view keeper_boss_victory_pages[] = {
        RPG_T("The lamp turns again.\nIn its base: a ship's\ncompass, still true.",
              "灯が、ふたたび回りだす。\n台座の中に、まだ狂いのない\n羅針盤があった。"),
        RPG_T("Got the Compass!",
              "羅針盤を手に入れた！"),
    };

    constexpr bn::string_view ryuze_pages[] = {
        RPG_T(RPG_NAME_GENERAL_4 ": You came by sea.\nThen the old man lit\nhis fires after all.",
              RPG_NAME_GENERAL_4_JA "「海から来たか。\n　あの老人は、やはり\n　狼煙を上げたのだな。」"),
        RPG_T(RPG_NAME_GENERAL_4 ": My wife. My son.\nThe Emperor swore the\nflame would return them.",
              RPG_NAME_GENERAL_4_JA "「妻と、息子だ。\n　焔帝は、炎が二人を\n　返すとちかった。」"),
        RPG_T(RPG_NAME_HERO ": Has it returned\nanyone?",
              RPG_NAME_HERO_JA "「だれか一人でも、\n　戻ってきたのか。」"),
        RPG_T(RPG_NAME_GENERAL_4 ": ...Do not make me\nanswer that.",
              RPG_NAME_GENERAL_4_JA "「……それを、\n　答えさせるな。」"),
    };

    constexpr bn::string_view ryuze_victory_pages[] = {
        RPG_T(RPG_NAME_GENERAL_4 ": I knew. I think I\nalways knew. I could\nnot stop hoping.",
              RPG_NAME_GENERAL_4_JA "「知っていた。きっと、\n　ずっと知っていた。\n　望むのをやめられなかった。」"),
        RPG_T(RPG_NAME_GENERAL_4 " falls. The north\nis free.",
              RPG_NAME_GENERAL_4_JA "がたおれた。\n北は解き放たれた。"),
    };

    constexpr bn::string_view drake_pages[] = {
        RPG_T(RPG_NAME_GENERAL_1 ": Another avenger.\nWhich village? I have\nburned so many.",
              RPG_NAME_GENERAL_1_JA "「また仇討ちか。\n　どの村だ。あまりに多く\n　焼いたのでな。」"),
        RPG_T(RPG_NAME_HERO ": " RPG_NAME_VILLAGE ".",
              RPG_NAME_HERO_JA "「" RPG_NAME_VILLAGE_JA "村だ。」"),
        RPG_T(RPG_NAME_GENERAL_1 ": ...No. Nothing.\nI do not remember it.",
              RPG_NAME_GENERAL_1_JA "「……いや。\n　何も思い出せん。」"),
        RPG_T(RPG_NAME_HERO ": That's fine. I cut\nyou down for the next\nvillage, not for mine.",
              RPG_NAME_HERO_JA "「それでいい。お前を\n　斬るのは、次の村の\n　ためだ。」"),
    };

    constexpr bn::string_view drake_victory_pages[] = {
        RPG_T(RPG_NAME_GENERAL_1 " falls. He never\ndid remember.",
              RPG_NAME_GENERAL_1_JA "がたおれた。\n最後まで、思い出さなかった。"),
        RPG_T(RPG_NAME_HERO ": ...It's quiet.\nI thought there would\nbe more.",
              RPG_NAME_HERO_JA "「……静かだな。\n　もっと何かあると\n　思っていた。」"),
        RPG_T(RPG_NAME_PARTNER ": That's all there\never is, partner.",
              RPG_NAME_PARTNER_JA "「そんなものだ、\n　相棒。」"),
    };

    constexpr bn::string_view sendoff_pages[] = {
        RPG_T("The whole village waits\nat the gate. No one\nsays goodbye.",
              "里じゅうの者が、門に\n集まっていた。だれも\n別れの言葉は言わない。"),
        RPG_T("Rico: Come back. All\nfour. I counted.",
              "リコ「帰ってきてね。\n　四人とも。数えたから。」"),
        RPG_T("That night, by the fire,\n" RPG_NAME_PARTNER " sits down first.",
              "その夜、焚き火のそばに\n最初にすわったのは\n" RPG_NAME_PARTNER_JA "だった。"),
        RPG_T(RPG_NAME_PARTNER ": One more, partner.\nThen we rest.",
              RPG_NAME_PARTNER_JA "「あと一人だ、相棒。\n　それで終わりだ。」"),
        RPG_T("- Chapter 9 -\nThe Flame Emperor's Castle",
              "- 第9章 -\n焔帝の城"),
    };

    constexpr EncounterGroup lighthouse_battle = { { EnemyId::drowned_keeper }, 1 };
    constexpr EncounterGroup ryuze_battle = { { EnemyId::general_4, EnemyId::ash_zealot }, 2 };
    constexpr EncounterGroup drake_battle = { { EnemyId::general_2_last, EnemyId::cult_knight }, 2 };
    // ---------------------------------------------------------------------------------------------------------------
    // Chapter 9 "The Flame Emperor's Castle"
    // ---------------------------------------------------------------------------------------------------------------
    #define EMPEROR RPG_NAME_EMPEROR

    constexpr bn::string_view circle_pages[] = {
        RPG_T("The ash circle stirs. A\npath of embers opens,\ntoward the tower.",
              "灰の円がざわめく。\n残り火の道が、\n塔へ向かって開いていく。"),
        RPG_T(NOX ": The Emperor's road.\nIt opens only to those\nhe wishes to see.",
              RPG_NAME_DEFECTOR_JA "「焔帝の道だ。\n　奴が会いたいと望む者に\n　しか、開かない。」"),
    };

    constexpr bn::string_view arrival_pages[] = {
        RPG_T("A castle of black glass.\nNo guards. No sound but\nthe fire above.",
              "黒いガラスの城。\n衛兵はいない。聞こえるのは、\n頭上の炎の音だけだ。"),
    };

    constexpr bn::string_view visions_pages[] = {
        RPG_T("The glass walls show a\nvillage. A quiet morning.\nA pail by a spring.",
              "ガラスの壁に、村が映る。\n静かな朝。\n泉のそばの手おけ。"),
        RPG_T(SEER ": " HERO ". It is only\nthe fire, remembering.",
              RPG_NAME_SEER_JA "「" HERO_JA "。あれは炎が\n　思い出しているだけです。」"),
        RPG_T(HERO ": I know. They are\nnot coming back.",
              HERO_JA "「分かっている。\n　ふたりは、帰ってこない。」"),
        RPG_T(HERO ": ...I know.",
              HERO_JA "「……分かっている。」"),
    };

    constexpr bn::string_view emperor_pages[] = {
        RPG_T(EMPEROR ": The ember of\n" RPG_NAME_VILLAGE ". I watched you,\nthe whole way.",
              RPG_NAME_EMPEROR_JA "「" VILLAGE_JA "の火の粉か。\n　ずっと見ていたぞ。\n　ここまでの道のりを。」"),
        RPG_T(EMPEROR ": I lost someone,\nonce. As you did. I would\nnot accept it.",
              RPG_NAME_EMPEROR_JA "「私もかつて、\n　ひとりを失った。お前と同じだ。\n　私は、認めなかった。」"),
        RPG_T(EMPEROR ": Feed a pyre the\nwhole world, and the\ndead walk out of it.",
              RPG_NAME_EMPEROR_JA "「火葬台に世界を\n　まるごとくべれば、\n　死者はそこから歩み出る。」"),
        RPG_T(HERO ": Eight parts in ten.\nDid they?",
              HERO_JA "「世界の八割だ。\n　それで、出てきたのか？」"),
        RPG_T(EMPEROR ": ...Not yet.",
              RPG_NAME_EMPEROR_JA "「……まだだ。」"),
        RPG_T(HERO ": I wanted that, too.\nI let a city burn,\nwanting it.",
              HERO_JA "「俺もそれを望んだ。\n　望んだせいで、\n　都をひとつ燃やした。」"),
        RPG_T(HERO ": They don't come\nback. It ends here.",
              HERO_JA "「死者は帰ってこない。\n　ここで終わりだ。」"),
    };

    constexpr bn::string_view emperor_victory_pages[] = {
        RPG_T(EMPEROR ": Then I will burn\nwhat is left of me.",
              RPG_NAME_EMPEROR_JA "「ならば、私に残った\n　ものを燃やそう。」"),
        RPG_T("The Emperor steps into\nhis own pyre.",
              "焔帝は、みずからの\n火葬台へ踏み入った。"),
        RPG_T(SEER ": The lamps... I can\nfeel them. All of them.\nStand up!",
              RPG_NAME_SEER_JA "「灯火が……感じます。\n　すべての灯火を。\n　立ってください！」"),
        RPG_T("The party's strength\nreturns!",
              "仲間たちに力がもどった！"),
    };

    constexpr bn::string_view pyre_pages[] = {
        RPG_T("The pyre rises, wearing\na crown of ash.",
              "火葬台が立ち上がる。\n灰の王冠をまとって。"),
    };

    constexpr bn::string_view pyre_victory_pages[] = {
        RPG_T("The pyre gutters.\nIn it, a man's voice.",
              "火葬台の炎がゆらぐ。\nその中から、男の声がした。"),
        RPG_T(EMPEROR ": She did not come.\nThen what was all this\nash for?",
              RPG_NAME_EMPEROR_JA "「彼女は来なかった。\n　ならば、この灰は\n　何のためだったのだ？」"),
        RPG_T(HERO ": Nothing. It is just\nwhat is left. We carry\nit. That's all.",
              HERO_JA "「何のためでもない。\n　ただ、残ったものだ。\n　俺たちが背負う。それだけだ。」"),
        RPG_T("The fire above the\ncastle goes out.",
              "城の上の炎が、消えた。"),
        RPG_T(PARTNER ": ...It's over.",
              PARTNER_JA "「……終わったな。」"),
        RPG_T(HERO ": No. Now we go\nhome, to what isn't\nthere.",
              HERO_JA "「いや。これから帰るんだ。\n　もう何もない、\n　あの場所へ。」"),
        RPG_T("- Final Chapter -\nGreen in the Ash",
              "- 最終章 -\n灰に芽吹く"),
    };

    constexpr EncounterGroup emperor_battle = { { EnemyId::emperor }, 1 };
    // ---------------------------------------------------------------------------------------------------------------
    // Chapter 10 "Green in the Ash"
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view voyage_pages[] = {
        RPG_T("The " SHIP " is ready.\nShe sails south, around\nthe " GATE ", toward home.",
              RPG_NAME_SHIP_JA "の準備はできた。\n船は南へ、" RPG_NAME_GATE_JA "を回って\n故郷へ向かう。"),
    };

    constexpr bn::string_view homecoming_pages[] = {
        RPG_T(RPG_NAME_VILLAGE ". Nothing has\nchanged. Ash, and two\ngraves.",
              VILLAGE_JA "村。\n何も変わっていない。\n灰と、ふたつの墓。"),
        RPG_T(HERO ": I killed him. I\nthought it would feel\nlike coming home.",
              HERO_JA "「あいつをたおした。\n　帰ってきた気持ちになれると\n　思っていたんだ。」"),
        RPG_T("In your pack, something\nis warm. An ember of the\nPyre. Still alive.",
              "荷物の中で、何かが温かい。\n火葬台の残り火だ。\nまだ、生きている。"),
    };

    constexpr bn::string_view grave_ember_pages[] = {
        RPG_T("The ember burns in your\nhand. A whisper: Feed me,\nand they walk out.",
              "手の中で残り火が燃える。\nささやき声がする。\n「くべよ。さすれば歩み出る」"),
        RPG_T(HERO ": ...Father. Mother.",
              HERO_JA "「……父さん。母さん。」"),
        RPG_T("For one breath, the fire\ngrows. The ash of " RPG_NAME_VILLAGE "\nbegins to rise.",
              "ひと息の間、炎が大きくなる。\n" VILLAGE_JA "村の灰が、\n舞い上がりはじめる。"),
        RPG_T(PARTNER ": " HERO "!",
              PARTNER_JA "「" HERO_JA "！」"),
        RPG_T(SEER ": Look down. Not at\nthe fire. Down.",
              RPG_NAME_SEER_JA "「下を見て。\n　炎ではなく。足もとを。」"),
        RPG_T("By the graves, in the\nash: one green shoot.",
              "墓のそば、灰の中に、\n緑の芽がひとつ。"),
        RPG_T(HERO ": ...No. They don't\ncome back.",
              HERO_JA "「……いや。\n　ふたりは、帰ってこない。」"),
        RPG_T(HERO " presses the ember\ninto the wet earth.\nIt goes out.",
              HERO_JA "は残り火を、\nしめった土に押しこんだ。\n火が消える。"),
        RPG_T(HERO ": But something\nelse can.",
              HERO_JA "「でも、別のものなら\n　芽を出せる。」"),
    };

    constexpr bn::string_view rebuild_pages[] = {
        RPG_T("Elder: You came back,\n" HERO ". All of you.",
              "長老「帰ってきたか、" HERO_JA "。\n　みな、そろってな。」"),
        RPG_T("Elder: We meant to\nrebuild. We had no\nhands, and no heart.",
              "長老「建て直すつもりじゃった。\n　だが人手も、気力も\n　なかったのじゃ。」"),
        RPG_T(HERO ": You have hands\nnow. Where do we start?",
              HERO_JA "「人手なら、今はある。\n　どこから始める？」"),
        RPG_T(NOX ": I know how things\nburn. Let me learn how\nthey are built.",
              RPG_NAME_DEFECTOR_JA "「燃やし方なら知っている。\n　建て方を、\n　学ばせてほしい。」"),
    };

    constexpr bn::string_view seasons_pages[] = {
        RPG_T("Seasons pass. Walls rise.\nThe well runs clear.\nPeople come, and stay.",
              "季節がめぐる。壁が立つ。\n井戸の水がすむ。\n人が来て、住みつく。"),
    };

    constexpr bn::string_view coronation_pages[] = {
        RPG_T("Elder: They have come\nfrom " REFUGE ", " RPG_NAME_MINE_TOWN ",\n" RPG_NAME_HARBOR ". All of them.",
              "長老「" RPG_NAME_REFUGE_JA "から、\n　" RPG_NAME_MINE_TOWN_JA "から、" RPG_NAME_HARBOR_JA "から。\n　みな、来てくれたぞ。」"),
        RPG_T("Keeper: We dug this out\nof " CAPITAL ". The realm\nneeds a king.",
              "灯守り「" RPG_NAME_CAPITAL_JA "から\n　掘り出してきたよ。\n　国には、王が要る。」"),
        RPG_T("A crown of gold, black\nwith soot.",
              "すすで黒ずんだ、\n金の王冠。"),
        RPG_T(HERO ": Not that one.",
              HERO_JA "「それじゃない。」"),
        RPG_T(SEER " weaves a ring of\ngreen shoots from the\nash, and holds it out.",
              RPG_NAME_SEER_JA "が、灰から出た緑の芽で\n輪をあみ、さし出した。"),
        RPG_T(PARTNER ": King of one\nvillage and a great\ndeal of ash.",
              PARTNER_JA "「村ひとつと、\n　山ほどの灰の王さまか。」"),
        RPG_T(HERO ": It's a start.",
              HERO_JA "「ここから始めるさ。」"),
        RPG_T(HERO " is crowned, in\nthe square of " RPG_NAME_VILLAGE ",\nbeside two graves.",
              HERO_JA "は、" VILLAGE_JA "村の広場で\n王冠をいただいた。\nふたつの墓のそばで。"),
        RPG_T(HERO ": Father. Mother.\nI'm home. You are\nnot here.",
              HERO_JA "「父さん。母さん。\n　ただいま。\n　ふたりは、ここにいない。」"),
        RPG_T(HERO ": ...That's all\nright.",
              HERO_JA "「……それで、いいんだ。」"),
        RPG_T("What burned will not\nreturn. What grows now\nis new.",
              "燃えたものは、戻らない。\n今、育つものは、\n新しいものだ。"),
        RPG_T("- THE END -\nThank you for playing.",
              "- おわり -\n遊んでくれてありがとう。"),
    };

    // ---------------------------------------------------------------------------------------------------------------
    // After the ending, part 1 "The Runaway Necklace"
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view post_stranger_pages[] = {
        RPG_T("Someone stands in the\nsquare, flickering like\na candle in the wind.",
              "広場に誰かが立っている。\n風の中のろうそくのように、\nゆらめきながら。"),
        RPG_T("???: It was morning. Then\nlast winter. Then this\nplace was ash. Is it ash?",
              "？？？「朝だった。次は去年の冬。\n　次は、ここが灰だった。\n　ここは、灰？」"),
        RPG_T(HERO ": Easy. This is " RPG_NAME_VILLAGE ".\nIt's rebuilt. You are\nstanding in it.",
              HERO_JA "「大丈夫だ。ここは" VILLAGE_JA "村だ。\n　建て直した。あんたは今、\n　その中に立っている。」"),
        RPG_T("???: The necklace. It\nslips. A little forward,\na little back. Sideways.",
              "？？？「首飾りが。すべるんだ。\n　少し先へ、少し昔へ。\n　それから、横へ。」"),
        RPG_T("???: Sideways is worst.\nA world where no one\nlit a single lamp.",
              "？？？「横がいちばんひどい。\n　誰ひとり灯火を\n　ともさなかった世界だ。」"),
        RPG_T(SEER ": Breathe. Look at\nme. One sky. This one.",
              RPG_NAME_SEER_JA "「息をして。私を見て。\n　空はひとつ。\n　この空だけです。」"),
        RPG_T(PARTNER ": Sit, before you\nfall through the floor.\nThere. Better.",
              PARTNER_JA "「座れ。床をすりぬけて\n　落ちる前にな。\n　……そうだ。それでいい。」"),
        RPG_T("The flickering slows.\nThe necklace still hums.",
              "ゆらめきが、ゆるやかになる。\n首飾りは、まだうなっている。"),
        RPG_T("???: ...Thank you. I\ncan't make it stop. I\ndon't know when I am.",
              "？？？「……ありがとう。\n　止められないんだ。自分が\n　いつにいるのか分からない。」"),
        RPG_T(SEER ": There are several\nskies in that stone. The\nElder of " RPG_NAME_SEER_TOWN " reads such.",
              RPG_NAME_SEER_JA "「その石の中には、空が\n　いくつもあります。" RPG_NAME_SEER_TOWN_JA "の\n　長なら、読み解けます。」"),
        RPG_T(HERO ": Then we ask her.\nStay here. You are safe\nin " RPG_NAME_VILLAGE ".",
              HERO_JA "「なら、長に頼もう。\n　ここにいてくれ。\n　" VILLAGE_JA "村なら安全だ。」"),
    };

    constexpr bn::string_view post_seer_elder_pages[] = {
        RPG_T("Elder Seer: " SEER ". You\nbring a strange star\nhome with you.",
              "星読みの長「" RPG_NAME_SEER_JA "。\n　ふしぎな星を\n　連れて帰ってきたね。」"),
        RPG_T(SEER ": A necklace that\nslips through time.\nAnd across worlds.",
              RPG_NAME_SEER_JA "「時をすべる首飾りです。\n　そして、世界のあいだも。」"),
        RPG_T("Elder Seer: Forward, back\nand aside. Three threads\nnever meant to be tied.",
              "星読みの長「先へ、昔へ、横へ。\n　結ばれるはずのなかった、\n　三本の糸だね。」"),
        RPG_T("Elder Seer: Its clasp is\n" RPG_NAME_SEER_TOWN " work. Older\nthan I am.",
              "星読みの長「この留め金は\n　" RPG_NAME_SEER_TOWN_JA "の細工だよ。\n　私よりも古い。」"),
        RPG_T("Elder Seer: I will read\nits sky. Come back when\nthe stars have turned.",
              "星読みの長「その空を読んで\n　みよう。星がめぐったころに、\n　またおいで。」"),
    };

    constexpr bn::string_view post_seer_elder_wait_pages[] = {
        RPG_T("Elder Seer: The reading\nis not done. Patience.",
              "星読みの長「読み解きは\n　まだ終わっていないよ。\n　あせらずにね。」"),
    };

    constexpr EncounterGroup pyre_battle = { { EnemyId::emperor_pyre }, 1 };


    constexpr EncounterGroup bardos_last_battle = { { EnemyId::bardos_last, EnemyId::cult_knight }, 2 };

    // Chapter 6: three battles in a row at the gate, then the general (stopped after 5 rounds).
    constexpr EncounterGroup gate_1_battle = { { EnemyId::cult_knight, EnemyId::cult_knight }, 2 };
    constexpr EncounterGroup gate_2_battle = { { EnemyId::ash_zealot, EnemyId::cult_knight, EnemyId::cult_knight }, 3 };
    constexpr EncounterGroup gate_3_battle = { { EnemyId::ash_zealot, EnemyId::ash_zealot, EnemyId::cult_knight }, 3 };
    constexpr EncounterGroup bardos_battle = { { EnemyId::general_2 }, 1 };
    constexpr int bardos_turn_limit = 5;

    // Chapter 5.
    constexpr EncounterGroup deserter_battle = { { EnemyId::cult_knight, EnemyId::cult_knight }, 2 };
    constexpr EncounterGroup overseer_battle = { { EnemyId::mine_overseer, EnemyId::cult_knight }, 2 };
    constexpr EncounterGroup zelma_battle = { { EnemyId::zelma, EnemyId::ash_zealot }, 2 };
    constexpr int zelma_join_turn = 3;      // E506: the partner comes back on the third round.
    constexpr int defector_min_level = 12;

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
            // Someone who comes back (see Party::leave()) keeps what it was wearing.
            PartyMember& member = party.members[party.size - 1];

            if(member.weapon == ItemId::none)
            {
                member.weapon = weapon;
            }

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

    // ----- Chapter 5 (E501-E507) -----
    case StoryId::ch5_dorn:
        say_pages(ui, dorn_arrival_pages);
        story_progress = progress::ch5_dorn;
        break;

    case StoryId::ch5_deserter:
        say_pages(ui, deserter_pages);
        result.battle = &deserter_battle;
        break;

    case StoryId::ch5_mine_gate:
        say_pages(ui, mine_gate_pages);
        result.step_back = true;
        break;

    case StoryId::ch5_overseer:
        say_pages(ui, overseer_pages);
        result.battle = &overseer_battle;
        break;

    case StoryId::ch5_kazami:
        say_pages(ui, kazami_arrival_pages);
        story_progress = progress::ch5_kazami;
        break;

    case StoryId::ch5_confession:
        {
            // E505: either answer makes the partner leave; stopping him is remembered (one line of E507).
            bool stopped;

            {
                MessageBox box(ui);
                box.say_pages(confession_pages);
                stopped = ask_choice(ui, RPG_T("Stop him", "止める"), RPG_T("Watch", "黙って見る"));
            }

            if(stopped)
            {
                state.flags.set(FlagId::stopped_shion);
                say_pages(ui, confession_stop_pages);
            }
            else
            {
                say_pages(ui, confession_watch_pages);
            }

            say_pages(ui, confession_leave_pages);
            state.party.leave(ClassId::lancer);     // He keeps his level and equipment until he returns.
            story_progress = progress::ch5_split;
        }
        break;

    case StoryId::ch5_cathedral_gate:
        say_pages(ui, cathedral_gate_pages);
        result.step_back = true;
        break;

    case StoryId::ch5_east_gate:
        say_pages(ui, east_gate_ch5_pages);
        result.step_back = true;
        break;

    case StoryId::ch5_zelma:
        // E506: the battle starts with three; the partner bursts in on the third round (story_battle_reinforce).
        if(story_progress < progress::ch5_cathedral)
        {
            say_pages(ui, zelma_pages);
            story_progress = progress::ch5_cathedral;
        }
        else
        {
            say_pages(ui, zelma_again_pages);   // After losing the battle.
        }

        result.battle = &zelma_battle;
        result.battle_join_turn = zelma_join_turn;
        break;

    // ----- Chapter 6 (E601-E605) -----
    case StoryId::ch6_audience:
        say_pages(ui, audience_pages);
        story_progress = progress::ch6_audience;
        break;

    case StoryId::ch6_inn:
        if(story_progress < progress::ch6_audience)
        {
            say_pages(ui, inn_before_pages);
        }
        else if(story_progress < progress::ch6_eve)
        {
            // E602: the last rest. At night the companions are out in the city.
            say_pages(ui, inn_rest_pages);
            ui.fade_out();
            state.party.restore_all();
            ui.wait(30);
            story_progress = progress::ch6_eve;
            result.reload_map = true;       // Fades in again, with the companions in place.
            result.next = StoryId::ch6_eve;
        }
        else if(story_progress < progress::ch6_gate)
        {
            // E603: three battles in a row, with no rest in between.
            bool sleep;

            {
                MessageBox box(ui);
                box.show(inn_sleep_pages[0]);
                sleep = ask_yes_no(ui);
            }

            if(sleep)
            {
                say_pages(ui, gate_1_pages);
                result.battle = &gate_1_battle;
            }
        }
        else
        {
            say_pages(ui, inn_before_pages);
        }
        break;

    case StoryId::ch6_eve:
        say_pages(ui, eve_pages);
        break;

    case StoryId::ch6_eve_partner:
    case StoryId::ch6_eve_seer:
    case StoryId::ch6_eve_defector:
        {
            // E602 (optional): talking to all three is remembered for a reward in chapter 7.
            if(id == StoryId::ch6_eve_partner)
            {
                say_pages(ui, eve_partner_pages);
                state.flags.set(FlagId::ch6_eve_partner);
            }
            else if(id == StoryId::ch6_eve_seer)
            {
                say_pages(ui, eve_seer_pages);
                state.flags.set(FlagId::ch6_eve_seer);
            }
            else
            {
                say_pages(ui, eve_defector_pages);
                state.flags.set(FlagId::ch6_eve_defector);
            }

            if(state.flags.get(FlagId::ch6_eve_partner) && state.flags.get(FlagId::ch6_eve_seer) &&
               state.flags.get(FlagId::ch6_eve_defector))
            {
                state.flags.set(FlagId::ch6_eve_done);
            }
        }
        break;

    case StoryId::ch6_gate_2:
        say_pages(ui, gate_2_pages);
        result.battle = &gate_2_battle;
        break;

    case StoryId::ch6_gate_3:
        say_pages(ui, gate_3_pages);
        result.battle = &gate_3_battle;
        break;

    case StoryId::ch6_palace:
        if(story_progress < progress::ch6_gate)
        {
            say_pages(ui, palace_closed_pages);
        }
        else
        {
            // Into the royal castle.
            ui.fade_out();
            state.map = MapId::throne;
            state.player_x = 9;
            state.player_y = 8;
            state.facing = Direction::up;
            result.reload_map = true;
        }
        break;

    case StoryId::ch6_great_gate:
        say_pages(ui, great_gate_pages);
        result.step_back = true;
        break;

    case StoryId::ch6_bardos:
        // E604: stopped after 5 rounds, or when the party falls. Either way the story goes on.
        say_pages(ui, bardos_pages);
        result.battle = &bardos_battle;
        result.battle_turn_limit = bardos_turn_limit;
        break;

    case StoryId::ch6_road_lost:
        say_pages(ui, road_lost_pages);
        result.step_back = true;
        break;

    // ----- Chapter 7 (E701-E705) -----
    case StoryId::ch7_awake:
        // E701: alone. The reward of the eve of the siege (E602) is given here.
        say_pages(ui, awake_pages);

        if(state.flags.get(FlagId::ch6_eve_done))
        {
            say_pages(ui, awake_charm_pages);
            state.party.add_item(ItemId::rico_charm);
        }

        story_progress = progress::ch7_awake;
        break;

    case StoryId::ch7_rico:
        say_pages(ui, story_progress < progress::ch7_seer ? bn::span<const bn::string_view>(rico_pages) :
                                                            bn::span<const bn::string_view>(rico_later_pages));
        break;

    case StoryId::ch7_seer:
        say_pages(ui, seer_rejoin_pages);
        state.party.join(ClassId::mage, seer_min_level);        // She comes back as she was (Party::away).
        story_progress = progress::ch7_seer;
        break;

    case StoryId::ch7_south_gate:
        say_pages(ui, south_gate_pages);
        result.step_back = true;
        break;

    case StoryId::ch7_keeper:
        if(story_progress == progress::ch8_start)
        {
            // E801: the three goals of chapter 8.
            say_pages(ui, goals_pages);
            story_progress = progress::ch8_goals;
        }
        else if(story_progress > progress::ch8_start && story_progress < progress::ch9_start)
        {
            if(state.flags.get(FlagId::ryuze_defeated) && state.flags.get(FlagId::drake_defeated))
            {
                result.next = StoryId::ch8_sendoff;     // E805 also starts here, for whoever missed the entrances.
            }
            else
            {
                say_pages(ui, goals_again_pages);
            }
        }
        else
        {
            say_pages(ui, story_progress < progress::ch8_start ? bn::span<const bn::string_view>(keeper_pages) :
                                                                 bn::span<const bn::string_view>(keeper_after_pages));
        }
        break;

    case StoryId::ch7_east_gate:
        say_pages(ui, east_gate_ch7_pages);
        result.step_back = true;
        break;

    case StoryId::ch7_defector:
        say_pages(ui, defector_rejoin_pages);
        state.party.join(ClassId::priest, defector_min_level);
        story_progress = progress::ch7_defector;
        break;

    case StoryId::ch7_partner:
        say_pages(ui, partner_rejoin_pages);
        state.party.join(ClassId::lancer, partner_min_level);
        story_progress = progress::ch7_partner;
        break;

    case StoryId::ch7_fort_gate:
        say_pages(ui, fort_gate_pages);
        result.step_back = true;
        break;

    case StoryId::ch7_campfire:
        // E703: the heart of the story. The night by the fire restores the party.
        say_pages(ui, campfire_pages);
        state.party.restore_all();
        story_progress = progress::ch7_vow;
        break;

    case StoryId::ch7_bardos:
        say_pages(ui, bardos_last_pages);
        result.battle = &bardos_last_battle;
        break;

    case StoryId::ch7_signal:
        say_pages(ui, signal_pages);
        story_progress = progress::ch8_start;
        break;

    // ----- Chapter 8 (E801-E805; E801 is in ch7_keeper) -----
    case StoryId::ch8_lighthouse_gate:
        say_pages(ui, lighthouse_gate_pages);
        result.step_back = true;
        break;

    case StoryId::ch8_lighthouse:
        say_pages(ui, keeper_boss_pages);
        result.battle = &lighthouse_battle;
        break;

    case StoryId::ch8_shipwright:
        say_pages(ui, story_progress < progress::ch8_ship ? bn::span<const bn::string_view>(shipwright_pages) :
                                                            bn::span<const bn::string_view>(shipwright_ready_pages));
        break;

    case StoryId::ch8_pier:
        if(story_progress < progress::ch8_ship)
        {
            say_pages(ui, pier_pages);
            result.step_back = true;
        }
        else
        {
            // The ship: north to the ice tower, or to the volcano fort. Both come back to this pier.
            int choice;

            {
                MessageBox box(ui);
                box.show(pier_ask_pages[0]);
                choice = ask_option(ui, RPG_T("Ice Tower", "氷壁の塔"), RPG_T("Volcano", "火山の砦"));
            }

            if(choice < 0)
            {
                result.step_back = true;
            }
            else
            {
                ui.fade_out();
                state.map = choice == 0 ? MapId::ice_tower : MapId::citadel;
                state.player_x = 9;
                state.player_y = choice == 0 ? 7 : 10;
                state.facing = Direction::up;
                result.reload_map = true;
            }
        }
        break;

    case StoryId::ch8_ryuze:
        say_pages(ui, ryuze_pages);
        result.battle = &ryuze_battle;
        break;

    case StoryId::ch8_drake:
        say_pages(ui, drake_pages);
        result.battle = &drake_battle;
        break;

    case StoryId::ch8_sendoff:
        // E805: only when both generals are dead (the triggers are at the entrances of the village).
        if(state.flags.get(FlagId::ryuze_defeated) && state.flags.get(FlagId::drake_defeated))
        {
            say_pages(ui, sendoff_pages);
            state.party.restore_all();
            story_progress = progress::ch9_start;
        }
        break;

    case StoryId::ch9_circle:
        // The ember road: straight into the castle (walk out of its door to come back).
        say_pages(ui, circle_pages);
        ui.fade_out(40);
        state.map = MapId::castle;
        state.player_x = 9;
        state.player_y = 13;
        state.facing = Direction::up;
        result.reload_map = true;
        result.next = story_progress < progress::ch9_visions ? StoryId::ch9_arrival : StoryId::none;
        break;

    case StoryId::ch9_arrival:
        say_pages(ui, arrival_pages);
        break;

    case StoryId::ch9_visions:
        say_pages(ui, visions_pages);
        story_progress = progress::ch9_visions;
        break;

    case StoryId::ch9_emperor:
        if(story_progress < progress::ch9_first_form)
        {
            say_pages(ui, emperor_pages);
            result.battle = &emperor_battle;
        }
        else
        {
            result.next = StoryId::ch9_pyre;    // The first form is already beaten: only the pyre is left.
        }
        break;

    case StoryId::ch9_pyre:
        say_pages(ui, pyre_pages);
        result.battle = &pyre_battle;
        break;

    case StoryId::ch10_voyage:
        // The ship takes the party home: the burnt village, just inside its gate.
        say_pages(ui, voyage_pages);
        ui.fade_out(60);
        ui.wait(60);
        state.map = MapId::village;
        state.player_x = 8;
        state.player_y = 17;
        state.facing = Direction::up;
        result.reload_map = true;
        result.next = story_progress < progress::ch10_home ? StoryId::ch10_homecoming : StoryId::none;
        break;

    case StoryId::ch10_homecoming:
        say_pages(ui, homecoming_pages);
        story_progress = progress::ch10_home;
        break;

    case StoryId::ch10_grave:
        say_pages(ui, grave_ember_pages);
        story_progress = progress::ch10_ember_out;
        break;

    case StoryId::ch10_elder:
        if(story_progress < progress::ch10_rebuilt)
        {
            // Rebuilding: the village becomes its rebuilt state, with the hero in front of the elder.
            say_pages(ui, rebuild_pages);
            ui.fade_out(60);
            ui.wait(60);
            story_progress = progress::ch10_rebuilt;
            state.player_x = 12;
            state.player_y = 10;
            state.facing = Direction::up;
            result.reload_map = true;
            result.next = StoryId::ch10_seasons;
        }
        else
        {
            // The coronation and the ending: the game is cleared, and goes on in the rebuilt village.
            say_pages(ui, coronation_pages);
            story_progress = progress::game_clear;
        }
        break;

    case StoryId::ch10_seasons:
        say_pages(ui, seasons_pages);
        break;

    case StoryId::post_stranger:
        say_pages(ui, post_stranger_pages);
        story_progress = progress::post_stranger;
        break;

    case StoryId::post_seer_elder:
        if(story_progress < progress::post_seer_elder)
        {
            say_pages(ui, post_seer_elder_pages);
            story_progress = progress::post_seer_elder;
        }
        else
        {
            say_pages(ui, post_seer_elder_wait_pages);  // Part 2 continues from here.
        }
        break;

    default:
        BN_ERROR("Invalid story id: ", int(id));
        break;
    }

    return result;
}

BgmId story_bgm(StoryId id)
{
    switch(id)
    {

    case StoryId::ch1_attack:       // The village burns.
    case StoryId::ch1_awakening:
    case StoryId::ch5_kazami:       // The partner's past.
    case StoryId::ch5_confession:
    case StoryId::ch6_eve:          // The night before the capital falls.
    case StoryId::ch7_awake:
    case StoryId::ch10_grave:       // The temptation of the ember.
        return BgmId::event_dark;

    case StoryId::ch7_campfire:     // The vow by the fire: the party is whole again.
    case StoryId::ch10_homecoming:
    case StoryId::ch10_elder:       // The coronation and the ending.
    case StoryId::ch10_seasons:
        return BgmId::event_hope;

    default:
        return BgmId::none;
    }
}

bool story_battle_reinforce(GameState& state, MessageBox& box)
{
    // E506: the partner comes back in the middle of the battle against Zelma (once: after a lost battle he is
    // already in the party).
    if(state.story_battle == StoryId::ch5_zelma && state.party.join(ClassId::lancer, partner_min_level))
    {
        box.say_pages(zelma_reinforce_pages);
        return true;
    }

    return false;
}

bool story_defeat_continues(StoryId id)
{
    return id == StoryId::ch6_bardos;
}

StoryResult run_story_after_battle(StoryId id, [[maybe_unused]] bool won, GameState& state, Ui& ui)
{
    StoryResult result;

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

    case StoryId::ch5_deserter:
        // E502: the defector comes along as a guest (he joins for good after the mine, E503).
        say_pages(ui, deserter_victory_pages);
        join_member(state.party, ClassId::priest, defector_min_level, ItemId::oak_staff);
        state.story_progress = progress::ch5_deserter;
        break;

    case StoryId::ch5_overseer:
        say_pages(ui, overseer_victory_pages);
        state.story_progress = progress::ch5_mine;
        break;

    case StoryId::ch5_zelma:
        // E507: she flees. One line depends on what the player chose in E505.
        say_pages(ui, zelma_victory_pages);
        say_pages(ui, state.flags.get(FlagId::stopped_shion) ? bn::span<const bn::string_view>(zelma_stopped_pages) :
                                                                bn::span<const bn::string_view>(zelma_watched_pages));
        say_pages(ui, zelma_end_pages);
        state.story_progress = progress::ch6_start;
        break;

    case StoryId::ch6_inn:
        result.next = StoryId::ch6_gate_2;      // E603: no rest between the battles.
        break;

    case StoryId::ch6_gate_2:
        result.next = StoryId::ch6_gate_3;
        break;

    case StoryId::ch6_gate_3:
        say_pages(ui, gate_held_pages);
        state.story_progress = progress::ch6_gate;
        break;

    case StoryId::ch7_bardos:
        // E704 + E705: the first Ash General falls; back in the village, the signal fires are lit.
        say_pages(ui, bardos_last_victory_pages);
        ui.fade_out(60);
        ui.wait(30);
        state.story_progress = progress::ch7_bardos;
        state.party.restore_all();
        state.return_to_start();
        result.reload_map = true;
        result.next = StoryId::ch7_signal;
        break;

    case StoryId::ch8_lighthouse:
        say_pages(ui, keeper_boss_victory_pages);
        state.party.add_item(ItemId::compass);
        state.story_progress = progress::ch8_ship;
        break;

    case StoryId::ch8_ryuze:
        say_pages(ui, ryuze_victory_pages);
        state.flags.set(FlagId::ryuze_defeated);
        break;

    case StoryId::ch8_drake:
        say_pages(ui, drake_victory_pages);
        state.flags.set(FlagId::drake_defeated);
        break;

    case StoryId::ch9_emperor:
        // Two battles in a row: the second one starts as soon as this scene ends.
        say_pages(ui, emperor_victory_pages);
        state.party.restore_all();
        state.story_progress = progress::ch9_first_form;
        result.next = StoryId::ch9_pyre;
        break;

    case StoryId::ch9_pyre:
        // The castle goes dark and the party is taken back to the harbor (the respawn point of the chapter).
        say_pages(ui, pyre_victory_pages);
        ui.fade_out(60);
        ui.wait(60);
        state.story_progress = progress::ch10_start;
        state.party.restore_all();
        state.return_to_start();
        result.reload_map = true;
        break;

    case StoryId::ch6_bardos:
        // E604 + E605: won, lost or stopped, the king dies, the castle falls and the party is scattered.
        say_pages(ui, bardos_after_pages);
        ui.fade_out(60);
        ui.wait(60);
        state.story_progress = progress::ch6_collapse;
        state.party.restore_all();

        // The party is scattered: the hero is alone until the others are found again (E702).
        state.party.leave(ClassId::lancer);
        state.party.leave(ClassId::mage);
        state.party.leave(ClassId::priest);

        // He wakes up in the village of the refugees (E701).
        state.map = MapId::refuge;
        state.player_x = 3;
        state.player_y = 6;
        state.facing = Direction::down;
        result.reload_map = true;
        result.next = StoryId::ch7_awake;
        break;

    default:
        break;
    }

    return result;
}

}
