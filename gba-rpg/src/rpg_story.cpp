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

    constexpr bn::string_view defector_pages[] = {
        RPG_T("A man in a cult robe sits\nalone. The miners give\nhim a wide berth.",
              "教団のローブを着た男が\nひとりで座っている。\n坑夫たちは近よらない。"),
        RPG_T(HERO ": That crest--!",
              HERO_JA "「その紋章は――！」"),
        RPG_T("???: Put the sword down.\nI took the robe off once.\nIt doesn't come off.",
              "？？？「剣を下ろせ。\n　このローブは一度ぬいだ。\n　だが、ぬげはしないのだ。」"),
        RPG_T(NOX ": I am " NOX ". I was a\npriest of the " RPG_NAME_CULT ".\nI lit their fires.",
              RPG_NAME_DEFECTOR_JA "「私は" RPG_NAME_DEFECTOR_JA "。\n　" RPG_NAME_CULT_JA "の司祭だった。\n　奴らの火をつけた者だ。」"),
        RPG_T(PARTNER ": Then burn.",
              PARTNER_JA "「なら、燃えろ。」"),
        RPG_T(NOX ": Soon enough. First:\n" GENERAL " prays at the Ash\nCathedral, east of here.",
              RPG_NAME_DEFECTOR_JA "「いずれな。その前に――\n　" GENERAL_JA "は東の\n　灰の聖堂で祈っている。」"),
        RPG_T(NOX ": Its doors open only\nto the seal of a priest.\nI can open them.",
              RPG_NAME_DEFECTOR_JA "「聖堂の扉は、司祭の\n　印章にしか開かない。\n　私なら開けられる。」"),
        RPG_T(NOX ": But I buried my seal\nin a ruin, north of here.\nWe need it first.",
              RPG_NAME_DEFECTOR_JA "「だが印章は、北の廃墟に\n　埋めてきた。\n　まず、それが要る。」"),
        RPG_T(SEER ": His star is dim.\nBut it is not lying.",
              RPG_NAME_SEER_JA "「彼の星は暗い。\n　けれど、うそは\n　ついていません。」"),
        RPG_T(HERO ": ...You walk in\nfront. Where I can\nsee you.",
              HERO_JA "「……お前は前を歩け。\n　俺から見えるところをな。」"),
        RPG_T(NOX " joined the party!",
              RPG_NAME_DEFECTOR_JA "が仲間になった！"),
    };

    constexpr bn::string_view north_gate_pages[] = {
        RPG_T("A road north, into ruins.\nThe miners say no one\nlives there anymore.",
              "北の廃墟へ続く道だ。\n坑夫たちの話では、\nもう誰も住んでいない。"),
    };

    constexpr bn::string_view east_gate_ch5_pages[] = {
        RPG_T("A black spire rises in\nthe east. Its doors are\nsealed to outsiders.",
              "東に黒い尖塔がそびえている。\nその扉は、よそ者には\n閉ざされている。"),
    };

    constexpr bn::string_view ruins_pages[] = {
        RPG_T("A single grave marker.\nSomeone tended it, once.",
              "墓標がひとつだけ。\nかつては誰かが、\n手入れをしていたようだ。"),
        RPG_T(PARTNER ": ...Stop.\nDon't touch that.",
              PARTNER_JA "「……やめろ。\n　それにさわるな。」"),
        RPG_T(PARTNER ": This is " RPG_NAME_PARTNER_HOME ".\nThis was my home.",
              PARTNER_JA "「ここは" RPG_NAME_PARTNER_HOME_JA "。\n　俺の故郷だった。」"),
        RPG_T(HERO ": You said you hunt\nfor bounties.",
              HERO_JA "「賞金かせぎだと\n　言っていただろう。」"),
        RPG_T(PARTNER ": A bounty is\nlighter to carry than\na grave.",
              PARTNER_JA "「賞金のほうが、\n　墓より軽く\n　背負えるからな。」"),
        RPG_T(NOX ": ...I know this\nplace. I held a torch\nhere, that night.",
              RPG_NAME_DEFECTOR_JA "「……この場所を知っている。\n　あの夜、私はここで\n　たいまつを持っていた。」"),
        RPG_T(PARTNER ": You.",
              PARTNER_JA "「……お前が。」"),
        RPG_T(PARTNER "'s spear is at\n" NOX "'s throat. " HERO "\nsteps between them.",
              PARTNER_JA "の槍が、" RPG_NAME_DEFECTOR_JA "の\nのどもとに突きつけられた。\n" HERO_JA "が間に割って入る。"),
        RPG_T(PARTNER ": You'd shield him?\nAfter " RPG_NAME_VILLAGE "? After your\nown dead?",
              PARTNER_JA "「こいつをかばうのか？\n　" VILLAGE_JA "村のあとで？\n　お前の家族が死んだのに？」"),
        RPG_T(HERO ": We need him to\nreach " GENERAL ".",
              HERO_JA "「" GENERAL_JA "にたどり着くには\n　こいつが必要だ。」"),
        RPG_T(PARTNER ": Then need him.\nI'm done needing anyone.",
              PARTNER_JA "「なら、必要としていろ。\n　俺はもう、誰も\n　必要としない。」"),
        RPG_T(PARTNER " left the party.",
              PARTNER_JA "がパーティを離れた。"),
        RPG_T(NOX ": The seal is here.\n...So is everything\nelse I buried.",
              RPG_NAME_DEFECTOR_JA "「印章はここにある。\n　……私が埋めた、ほかの\n　すべてのものと一緒に。」"),
    };

    constexpr bn::string_view cathedral_pages[] = {
        RPG_T("The doors open at " NOX "'s\ntouch. " GENERAL " kneels\nbefore a pyre.",
              RPG_NAME_DEFECTOR_JA "が触れると、扉が開く。\n" GENERAL_JA "が火葬台の前に\nひざまずいていた。"),
        RPG_T(GENERAL ": The traitor, the\nstray, the stargazer. And\nno spear. Did he break?",
              GENERAL_JA "「裏切り者に、のら犬に、\n　星読みか。槍がおらんな。\n　折れたか？」"),
        RPG_T(HERO ": He's not here.\nI am.",
              HERO_JA "「あいつはいない。\n　俺がいる。」"),
        RPG_T("A spear strikes the floor\nbetween them.",
              "一本の槍が、\n二人の間の床に突き立った。"),
        RPG_T(PARTNER ": I didn't come\nfor you, " HERO ".",
              PARTNER_JA "「お前のために\n　来たんじゃないぞ、" HERO_JA "。」"),
        RPG_T(PARTNER ": " RPG_NAME_PARTNER_HOME " deserves\nbetter than a son who\nruns. Move over.",
              PARTNER_JA "「" RPG_NAME_PARTNER_HOME_JA "に、逃げる息子は\n　ふさわしくない。\n　そこをあけろ。」"),
        RPG_T(PARTNER " rejoined the party!",
              PARTNER_JA "が仲間に戻った！"),
    };

    constexpr bn::string_view cathedral_again_pages[] = {
        RPG_T(PARTNER ": Again.\nHe falls today.",
              PARTNER_JA "「もう一度だ。\n　奴は今日、たおれる。」"),
    };

    constexpr bn::string_view cathedral_victory_pages[] = {
        RPG_T(GENERAL ": So the ash\nremembers its dead...",
              GENERAL_JA "「灰は、死者を\n　覚えているというのか……」"),
        RPG_T(GENERAL ": I gave the orders.\nBut the fire is his.\nIt will eat you, too.",
              GENERAL_JA "「命じたのは私だ。\n　だが炎は焔帝さまのもの。\n　貴様らも食われるぞ。」"),
        RPG_T(GENERAL " vanishes into the\nsmoke. The pyre goes out.",
              GENERAL_JA "は煙の中へ消えた。\n火葬台の火が消える。"),
        RPG_T(PARTNER ": ...Gone. Again.",
              PARTNER_JA "「……また、\n　逃げられたか。」"),
        RPG_T(PARTNER ": " NOX ". I won't\nforgive you. But I'll\nstand next to you.",
              PARTNER_JA "「" RPG_NAME_DEFECTOR_JA "。お前を許しは\n　しない。だが、\n　となりには立ってやる。」"),
        RPG_T(NOX ": That is more than\nI will ever ask.",
              RPG_NAME_DEFECTOR_JA "「それは、私が望める\n　以上のことだ。」"),
        RPG_T(HERO ": He bleeds. Next\ntime, he falls.",
              HERO_JA "「奴も血を流す。\n　次は、たおす。」"),
        RPG_T("- Chapter 6 -\nThe Sinking Capital",
              "- 第6章 -\n沈む王都"),
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Chapter 6 "The Sinking Capital"
    // ---------------------------------------------------------------------------------------------------------------
    #define CAPITAL RPG_NAME_CAPITAL
    #define GENERAL2 RPG_NAME_GENERAL_2

    constexpr bn::string_view captain_pages[] = {
        RPG_T("Captain: Halt! You came by\nthe pilgrim road? Then\nthe Cathedral has fallen.",
              "隊長「止まれ！ 巡礼路から\n　来たのか？ ならば、\n　聖堂は落ちたのだな。」"),
        RPG_T(HERO ": " GENERAL " ran from it.",
              HERO_JA "「" GENERAL_JA "は逃げた。」"),
        RPG_T("Captain: Then hear this.\n" CAPITAL " is the last city\nstanding. And they come.",
              "隊長「ならば聞け。" RPG_NAME_CAPITAL_JA "は\n　最後に残った都だ。\n　そこへ奴らが来る。」"),
        RPG_T("Captain: General " GENERAL2 "\nleads ten thousand to our\neast wall. Tonight.",
              "隊長「将軍" RPG_NAME_GENERAL_2_JA "が一万の兵で\n　東の城壁に迫っている。\n　今夜だ。」"),
        RPG_T(SEER ": The stars over this\ncity... I cannot find\nthem.",
              RPG_NAME_SEER_JA "「この都の上の星が……\n　見つかりません。」"),
        RPG_T(HERO ": A general. Here.\nGood.",
              HERO_JA "「将軍が、ここに。\n　……ちょうどいい。」"),
        RPG_T(PARTNER ": " HERO ". We hold\nthe wall. We don't hunt.",
              PARTNER_JA "「" HERO_JA "。俺たちは城壁を\n　守るんだ。\n　狩りに来たんじゃない。」"),
        RPG_T("Captain: Rest, arm, then\ncome to the east gate.\nWe need every blade.",
              "隊長「休んで、装備を整えたら\n　東門へ来てくれ。\n　剣は一本でも多く要る。」"),
    };

    constexpr bn::string_view captain_again_pages[] = {
        RPG_T("Captain: The east gate,\nwhen you are ready.\nThere is little time.",
              "隊長「準備ができたら東門へ。\n　時間はあまりない。」"),
    };

    constexpr bn::string_view east_gate_closed_pages[] = {
        RPG_T("The east gate. Guards\nwave you back from\nthe wall.",
              "東門だ。衛兵が手をふって、\n城壁から下がれと\n合図している。"),
    };

    constexpr bn::string_view siege_pages[] = {
        RPG_T("Horns. The plain beyond\nthe wall is a sea of\ntorches.",
              "角笛が鳴る。\n城壁の向こうの平原は、\nたいまつの海だった。"),
        RPG_T("Captain: Hold the gate!\nHold--",
              "隊長「門を守れ！\n　守り抜――」"),
        RPG_T("The wall does not break.\nIt melts.",
              "城壁は、くずれなかった。\n溶けたのだ。"),
    };

    constexpr bn::string_view burning_pages[] = {
        RPG_T("Within the hour, " CAPITAL "\nis burning.",
              "一時間もたたずに、\n" RPG_NAME_CAPITAL_JA "は燃えていた。"),
        RPG_T(NOX ": The palace. She will\ngo for the king. That is\nhow it is done.",
              RPG_NAME_DEFECTOR_JA "「宮殿だ。彼女は王を\n　ねらう。それが\n　奴らのやり方だ。」"),
    };

    constexpr bn::string_view general2_pages[] = {
        RPG_T(GENERAL2 ": So you are the\nember that burned " GENERAL ".",
              RPG_NAME_GENERAL_2_JA "「あなたが、" GENERAL_JA "を\n　焼いた火の粉ね。」"),
        RPG_T(GENERAL2 ": " RPG_NAME_VILLAGE ", was it?\nI signed that order.\nA small one. I forget it.",
              RPG_NAME_GENERAL_2_JA "「" VILLAGE_JA "村、だったかしら。\n　あの命令書に署名したのは私。\n　小さな村よ。もう忘れたわ。」"),
        RPG_T(PARTNER ": " HERO ", don't!\nPeople are still getting\nout. Hold the line!",
              PARTNER_JA "「" HERO_JA "、やめろ！\n　まだ人が逃げている。\n　戦列を守れ！」"),
        RPG_T(HERO ": ...She signed it.",
              HERO_JA "「……あいつが、\n　署名した。」"),
        RPG_T(HERO " breaks from the\nline and charges alone.",
              HERO_JA "は戦列を飛び出し、\nひとりで斬りかかった。"),
    };

    constexpr bn::string_view rout_lost_pages[] = {
        RPG_T("The flames close in.\nYour sword arm will\nnot rise.",
              "炎がせまってくる。\n剣を持つ腕が、\nもう上がらない。"),
    };

    constexpr bn::string_view rout_won_pages[] = {
        RPG_T(GENERAL2 " steps back,\nunhurt, and laughs. Behind\nher, the palace roof falls.",
              RPG_NAME_GENERAL_2_JA "は無傷のまま\n一歩下がり、笑った。\nその背後で宮殿の屋根が落ちる。"),
    };

    constexpr bn::string_view rout_pages[] = {
        RPG_T("The line you left is\ngone. So are the people\nbehind it.",
              "離れてきた戦列は、\nもうない。その後ろに\nいた人々も。"),
        RPG_T("Hands drag you away.\nBehind you, the palace\nsinks into the fire.",
              "いくつもの手に\n引きずられていく。背後で、\n宮殿が炎に沈んでいく。"),
        RPG_T(GENERAL2 ": Run, ember. Tell\nthe ashes what you saw.",
              RPG_NAME_GENERAL_2_JA "「逃げなさい、火の粉。\n　見たものを、\n　灰に語ってやるといいわ。」"),
    };

    constexpr bn::string_view aftermath_pages[] = {
        RPG_T(RPG_NAME_MINE_TOWN ". Three days later.\n" CAPITAL " is gone.",
              RPG_NAME_MINE_TOWN_JA "。三日後。\n" RPG_NAME_CAPITAL_JA "は、もうない。"),
        RPG_T(PARTNER ": The gate held\nuntil you left it. Do you\nknow how many we lost?",
              PARTNER_JA "「門は持ちこたえていた。\n　お前が離れるまではな。\n　何人死んだか分かるか？」"),
        RPG_T(HERO ": ...I saw her, and\nI couldn't see anything\nelse.",
              HERO_JA "「……あいつを見たら、\n　ほかには何も\n　見えなくなった。」"),
        RPG_T(PARTNER ": You didn't fight\nfor " CAPITAL ". You fought\nfor " RPG_NAME_VILLAGE ". Again.",
              PARTNER_JA "「お前は" RPG_NAME_CAPITAL_JA "のために\n　戦ったんじゃない。\n　また" VILLAGE_JA "村のためだ。」"),
        RPG_T(SEER ": " HERO ". I looked for\nyour star last night.\nIt was not there.",
              RPG_NAME_SEER_JA "「" HERO_JA "。昨夜、あなたの\n　星を探しました。\n　どこにも、なかった。」"),
        RPG_T(HERO ": ...I couldn't\nsave anything.",
              HERO_JA "「……何も、\n　守れなかった。」"),
        RPG_T(NOX ": The survivors went\nsouth, into the hills.\nThey will need hands.",
              RPG_NAME_DEFECTOR_JA "「生き残りは南の丘へ\n　向かった。\n　人手がいるはずだ。」"),
        RPG_T("- Chapter 7 -\nThe Lamplight",
              "- 第7章 -\n灯火"),
    };

    constexpr bn::string_view road_lost_pages[] = {
        RPG_T("The pilgrim road is choked\nwith smoke. There is no\ngoing back.",
              "巡礼路は煙でふさがっている。\nもう、戻れない。"),
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Chapter 7 "The Lamplight"
    // ---------------------------------------------------------------------------------------------------------------
    constexpr bn::string_view south_gate_pages[] = {
        RPG_T("A track south, into the\nhills. No reason to\ntake it yet.",
              "南の丘へ続く小道だ。\n今はまだ、\n行く理由がない。"),
    };

    constexpr bn::string_view keeper_pages[] = {
        RPG_T("Lamps hang from every\nbranch. The fire never\nreached this valley.",
              "どの枝にも灯火が\n下がっている。この谷に、\n炎は届かなかったのだ。"),
        RPG_T("Keeper: " CAPITAL " folk, all\nof us. You were on the\nwall. I remember you.",
              "灯守り「ここの者は皆、\n　" RPG_NAME_CAPITAL_JA "の民だよ。城壁に\n　いたね。覚えているよ。」"),
        RPG_T(HERO ": Then you know\nI left it.",
              HERO_JA "「なら、俺が持ち場を\n　離れたことも知っているな。」"),
        RPG_T("Keeper: I know my grandson\nran through that gate\nwhile it still stood.",
              "灯守り「知っているのは、\n　門がまだ立っていた間に、\n　孫が逃げのびたことさ。」"),
        RPG_T("Keeper: Each night we\nlight one lamp for\nsomeone lost.",
              "灯守り「わたしらは毎晩、\n　失われた誰かのために\n　灯火をひとつともす。」"),
        RPG_T("Keeper: Take this. Light\nit at the shrine, when you\nknow who it is for.",
              "灯守り「これをお持ち。\n　誰のための灯か分かったら、\n　祠でともしなさい。」"),
        RPG_T(HERO " took the lantern.",
              HERO_JA "は灯火を受け取った。"),
    };

    constexpr bn::string_view keeper_again_pages[] = {
        RPG_T("Keeper: The shrine is up\nthe path, in the north-\neast of the valley.",
              "灯守り「祠は道をのぼった先、\n　谷の北東にあるよ。」"),
    };

    constexpr bn::string_view keeper_after_pages[] = {
        RPG_T("Keeper: A lamp is small.\nThat is why it takes\nso many of us.",
              "灯守り「灯火は小さい。\n　だから、こんなに大勢が\n　要るのさ。」"),
    };

    constexpr bn::string_view shrine_pages[] = {
        RPG_T(HERO " lights the lantern.",
              HERO_JA "は灯火をともした。"),
        RPG_T(HERO ": For Father. For\nMother. ...For " CAPITAL ".",
              HERO_JA "「父さんに。母さんに。\n　……" RPG_NAME_CAPITAL_JA "に。」"),
        RPG_T(PARTNER ": I lit one here for\n" RPG_NAME_PARTNER_HOME ", years ago.\nNever told anyone.",
              PARTNER_JA "「何年も前、俺もここで\n　" RPG_NAME_PARTNER_HOME_JA "のためにともした。\n　誰にも言わなかったがな。」"),
        RPG_T(NOX ": May I light one?\nFor those I burned.",
              RPG_NAME_DEFECTOR_JA "「私も、ひとつ\n　ともしていいだろうか。\n　私が焼いた者たちのために。」"),
        RPG_T(SEER ": " HERO ". Your star\nis back. Small. But\nit is there.",
              RPG_NAME_SEER_JA "「" HERO_JA "。あなたの星が\n　戻りました。小さい。\n　けれど、そこにあります。」"),
        RPG_T(HERO ": I had all of you\nbeside me, and I fought\nalone. I'm sorry.",
              HERO_JA "「みんなが隣にいたのに、\n　俺はひとりで戦った。\n　すまなかった。」"),
        RPG_T(HERO ": No more revenge.\nI want there to be lamps\nleft to light.",
              HERO_JA "「復讐はもういい。\n　ともす灯火が残る世界で\n　あってほしいんだ。」"),
        RPG_T(PARTNER ": ...Took you\nlong enough.",
              PARTNER_JA "「……やっと、\n　分かったか。」"),
        RPG_T("A runner: Torches in the\neast pass! " GENERAL "'s host,\nhunting us!",
              "伝令「東の峠にたいまつだ！\n　" GENERAL_JA "の軍勢が、\n　俺たちを狩りに来た！」"),
        RPG_T(HERO ": Not this time.\nTogether.",
              HERO_JA "「今度はちがう。\n　一緒にだ。」"),
    };

    constexpr bn::string_view east_gate_ch7_pages[] = {
        RPG_T("The east pass. The lamps\nare kept dark on\nthis side.",
              "東の峠だ。こちら側では、\n灯火は消してある。"),
    };

    constexpr bn::string_view pass_pages[] = {
        RPG_T(GENERAL ": The ember. Come.\nCharge me, as you\ncharged her.",
              GENERAL_JA "「火の粉か。来い。\n　" RPG_NAME_GENERAL_2_JA "に斬りかかったように、\n　私にも来るがいい。」"),
        RPG_T(HERO ": No.",
              HERO_JA "「いいや。」"),
        RPG_T(HERO ": " PARTNER ", left.\n" SEER ", light. " NOX ", with\nme. We hold the line.",
              HERO_JA "「" PARTNER_JA "は左。" RPG_NAME_SEER_JA "、光を。\n　" RPG_NAME_DEFECTOR_JA "は俺と来い。\n　戦列を守るぞ。」"),
        RPG_T(GENERAL ": ...So. You have\nlearned to burn slow.",
              GENERAL_JA "「……そうか。\n　ゆっくり燃えることを\n　覚えたな。」"),
    };

    constexpr bn::string_view pass_victory_pages[] = {
        RPG_T(GENERAL ": A wall... I broke\na hundred walls...",
              GENERAL_JA "「壁、か……\n　私は百の壁を\n　くだいてきたのだが……」"),
        RPG_T(GENERAL " falls, and does\nnot rise. The first of\nthe four.",
              GENERAL_JA "はたおれ、\n二度と立たなかった。\n四人のうちの、最初のひとり。"),
        RPG_T(HERO ": It doesn't bring\nthem back.",
              HERO_JA "「それでも、みんなは\n　帰ってこない。」"),
        RPG_T(PARTNER ": No. But look\nbehind you.",
              PARTNER_JA "「ああ。だが、\n　後ろを見てみろ。」"),
        RPG_T("Down in the valley,\nevery lamp is still lit.",
              "谷では、すべての灯火が\nまだともっていた。"),
        RPG_T("- Chapter 8 -\nCounterattack",
              "- 第8章 -\n反攻"),
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Chapter 8 "Counterattack"
    // ---------------------------------------------------------------------------------------------------------------
    #define REFUGE RPG_NAME_REFUGE
    #define SHIP RPG_NAME_SHIP
    #define GENERAL3 RPG_NAME_GENERAL_3
    #define GENERAL4 RPG_NAME_GENERAL_4

    constexpr bn::string_view shipwright_pages[] = {
        RPG_T("A shipwright planes a\nhull, alone, on a dead\nwaterfront.",
              "人けのない波止場で、\n船大工がひとり、\n船体にかんなをかけている。"),
        RPG_T("Shipwright: The " SHIP ". Last\nhull in " RPG_NAME_HARBOR ". No crew,\nno sail, no reason.",
              "船大工「" RPG_NAME_SHIP_JA "だ。\n　" RPG_NAME_HARBOR_JA "最後の船さ。乗り手も\n　帆も、出す理由もない。」"),
        RPG_T(HERO ": The generals sit in\nthe citadel across the\nstrait. That's a reason.",
              HERO_JA "「将軍たちは、海峡の\n　向こうの城塞にいる。\n　それが理由だ。」"),
        RPG_T("Shipwright: I need forty\nhands to launch her.\nI have two.",
              "船大工「進水させるには\n　人手が四十は要る。\n　いるのは、おれひとりだ。」"),
        RPG_T(SEER ": Forty lamps burn\nin " REFUGE ".",
              RPG_NAME_SEER_JA "「" RPG_NAME_REFUGE_JA "には、\n　四十の灯火が\n　ともっています。」"),
        RPG_T(HERO ": Then we ask them.",
              HERO_JA "「なら、頼みに行こう。」"),
    };

    constexpr bn::string_view shipwright_wait_pages[] = {
        RPG_T("Shipwright: Forty hands.\nBring them, and the\n" SHIP " sails.",
              "船大工「人手が四十だ。\n　連れてくれば、\n　" RPG_NAME_SHIP_JA "は海に出る。」"),
    };

    constexpr bn::string_view shipwright_launch_pages[] = {
        RPG_T("The waterfront is full of\nvoices. " REFUGE " came.\nAnd " RPG_NAME_MINE_TOWN ". And more.",
              "波止場は人の声であふれて\nいた。" RPG_NAME_REFUGE_JA "が来た。\n" RPG_NAME_MINE_TOWN_JA "も。さらに大勢が。"),
        RPG_T("Shipwright: Well, I'll be.\nHaul! HAUL!",
              "船大工「こいつはおどろいた。\n　引け！ 引けえ！」"),
        RPG_T("The " SHIP " slides into\nthe water.",
              RPG_NAME_SHIP_JA "が、\n海へすべり出していく。"),
        RPG_T(PARTNER ": An army of\nbakers and miners.",
              PARTNER_JA "「パン屋と坑夫の\n　軍隊か。」"),
        RPG_T(HERO ": An army.",
              HERO_JA "「軍隊だ。」"),
        RPG_T("The " SHIP " waits at\nthe pier.",
              RPG_NAME_SHIP_JA "は、\n桟橋で待っている。"),
    };

    constexpr bn::string_view shipwright_after_pages[] = {
        RPG_T("Shipwright: She's yours.\nBring her back, or\ndon't come back.",
              "船大工「船はあんたらのものだ。\n　連れて帰ってこい。でなきゃ\n　帰ってくるな。」"),
    };

    constexpr bn::string_view keeper_rally_pages[] = {
        RPG_T(HERO ": We have a ship and\nno crew. I'm asking you\nall to stop hiding.",
              HERO_JA "「船はある。乗り手がいない。\n　みんなに頼みたい。\n　隠れるのを、やめてくれ。」"),
        RPG_T("Keeper: You ask a great\ndeal, wall-boy.",
              "灯守り「ずいぶんなことを\n　頼むね、城壁の坊や。」"),
        RPG_T("Keeper: ...But a lamp kept\nunder a bowl lights\nnothing. We will come.",
              "灯守り「……だが、鉢をかぶせた\n　灯火は何も照らさない。\n　行くよ、わたしらも。」"),
        RPG_T("The valley empties\ntoward the sea.",
              "谷の人々が、\n海へ向かって動き出す。"),
    };

    constexpr bn::string_view pier_pages[] = {
        RPG_T("A long pier. No ship\nis moored here.",
              "長い桟橋だ。\n船は一隻もつながれていない。"),
    };

    constexpr bn::string_view general3_pages[] = {
        RPG_T(GENERAL3 ": Bakers and miners,\non a boat. I will enjoy\nthe quiet after.",
              RPG_NAME_GENERAL_3_JA "「パン屋と坑夫が、船でか。\n　片づけたあとの静けさが\n　楽しみだ。」"),
        RPG_T(HERO ": Together. Go!",
              HERO_JA "「一緒にだ。行くぞ！」"),
    };

    constexpr bn::string_view general3_victory_pages[] = {
        RPG_T(GENERAL3 " falls. The second\nof the four.",
              RPG_NAME_GENERAL_3_JA "がたおれた。\n四人のうちの、ふたりめ。"),
    };

    constexpr bn::string_view general4_pages[] = {
        RPG_T(GENERAL4 ": Two of us dead?\nThen I will take both\ntheir shares of you.",
              RPG_NAME_GENERAL_4_JA "「ふたりも死んだの？\n　なら、ふたりの取り分まで\n　私がもらうわ。」"),
    };

    constexpr bn::string_view general4_victory_pages[] = {
        RPG_T(GENERAL4 " falls.\nOne is left.",
              RPG_NAME_GENERAL_4_JA "がたおれた。\n残るは、ひとり。"),
    };

    constexpr bn::string_view general2_last_pages[] = {
        RPG_T(GENERAL2 ": The ember.\nYou did not charge.",
              RPG_NAME_GENERAL_2_JA "「火の粉ね。\n　斬りかかってこないの。」"),
        RPG_T(HERO ": I'm not alone\nthis time.",
              HERO_JA "「今度は、\n　ひとりじゃない。」"),
        RPG_T(GENERAL2 ": No. You brought\nthe whole hearth.",
              RPG_NAME_GENERAL_2_JA "「そうね。かまどごと\n　連れてきたのね。」"),
    };

    constexpr bn::string_view general2_last_victory_pages[] = {
        RPG_T(GENERAL2 ": ...He never needed\ngenerals. We were\nkindling, too.",
              RPG_NAME_GENERAL_2_JA "「……あの方に、将軍など\n　要らなかった。私たちも、\n　たきつけだったのよ。」"),
        RPG_T(GENERAL2 " falls. The four\nAsh Generals are gone.",
              RPG_NAME_GENERAL_2_JA "がたおれた。\n四人の灰の将軍は、\nもういない。"),
        RPG_T(NOX ": The Emperor is alone\nin his tower now. He\nwill not mind.",
              RPG_NAME_DEFECTOR_JA "「焔帝は今、塔にひとりだ。\n　気にもとめないだろうが。」"),
        RPG_T(HERO ": Then we go to\nthe tower.",
              HERO_JA "「なら、塔へ行く。」"),
        RPG_T("- Chapter 9 -\nThe Flame Emperor's Castle",
              "- 第9章 -\n焔帝の城"),
    };

    constexpr EncounterGroup general3_battle = { { EnemyId::general_3, EnemyId::cult_knight }, 2 };
    constexpr EncounterGroup general4_battle = { { EnemyId::general_4, EnemyId::ash_zealot }, 2 };
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

    constexpr EncounterGroup pyre_battle = { { EnemyId::emperor_pyre }, 1 };

    constexpr EncounterGroup general2_last_battle = { { EnemyId::general_2_last }, 1 };

    constexpr EncounterGroup pass_battle = { { EnemyId::drake_last, EnemyId::ash_zealot }, 2 };

    constexpr EncounterGroup general2_battle = { { EnemyId::general_2 }, 1 };

    constexpr EncounterGroup cathedral_battle = { { EnemyId::drake_true, EnemyId::ash_zealot }, 2 };
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

    case StoryId::ch5_defector:
        say_pages(ui, defector_pages);
        join_member(state.party, ClassId::priest, defector_min_level, ItemId::oak_staff);
        story_progress = progress::ch5_defector;
        break;

    case StoryId::ch5_north_gate:
        say_pages(ui, north_gate_pages);
        result.step_back = true;
        break;

    case StoryId::ch5_ruins:
        // The party splits: the partner is gone (his equipment goes to the bag) until the cathedral.
        say_pages(ui, ruins_pages);
        state.party.leave(ClassId::lancer);
        story_progress = progress::ch5_split;
        break;

    case StoryId::ch5_east_gate:
        say_pages(ui, east_gate_ch5_pages);
        result.step_back = true;
        break;

    case StoryId::ch5_cathedral:
        if(story_progress < progress::ch5_reunited)
        {
            say_pages(ui, cathedral_pages);
            state.party.remove_item(ItemId::iron_spear);    // He takes back the spear he left behind, if it's still there.
            join_member(state.party, ClassId::lancer, partner_min_level, ItemId::iron_spear);
            story_progress = progress::ch5_reunited;
        }
        else
        {
            say_pages(ui, cathedral_again_pages);   // After losing the battle.
        }

        result.battle = &cathedral_battle;
        break;

    case StoryId::ch6_captain:
        if(story_progress < progress::ch6_muster)
        {
            say_pages(ui, captain_pages);
            story_progress = progress::ch6_muster;
        }
        else
        {
            say_pages(ui, captain_again_pages);
        }
        break;

    case StoryId::ch6_east_gate:
        if(story_progress < progress::ch6_muster)
        {
            say_pages(ui, east_gate_closed_pages);
            result.step_back = true;
        }
        else
        {
            // The siege: the capital becomes its burning state, with the party just inside the fallen wall.
            say_pages(ui, siege_pages);
            ui.fade_out(60);
            ui.wait(60);
            story_progress = progress::ch6_siege;
            state.player_x = 21;
            state.player_y = 6;
            state.facing = Direction::left;
            result.reload_map = true;
            result.next = StoryId::ch6_burning;
        }
        break;

    case StoryId::ch6_burning:
        say_pages(ui, burning_pages);
        break;

    case StoryId::ch6_general:
        say_pages(ui, general2_pages);
        result.battle = &general2_battle;
        break;

    case StoryId::ch6_aftermath:
        say_pages(ui, aftermath_pages);
        story_progress = progress::ch7_start;
        break;

    case StoryId::ch6_road_lost:
        say_pages(ui, road_lost_pages);
        result.step_back = true;
        break;

    case StoryId::ch7_south_gate:
        say_pages(ui, south_gate_pages);
        result.step_back = true;
        break;

    case StoryId::ch7_keeper:
        if(story_progress < progress::ch7_lantern)
        {
            say_pages(ui, keeper_pages);
            story_progress = progress::ch7_lantern;
        }
        else if(story_progress == progress::ch8_ship_found)
        {
            // Chapter 8: the refugees are asked to crew the ship.
            say_pages(ui, keeper_rally_pages);
            story_progress = progress::ch8_rallied;
        }
        else
        {
            say_pages(ui, story_progress < progress::ch7_resolve ? bn::span<const bn::string_view>(keeper_again_pages) :
                                                                   bn::span<const bn::string_view>(keeper_after_pages));
        }
        break;

    case StoryId::ch7_shrine:
        say_pages(ui, shrine_pages);
        state.party.restore_all();      // The night in the valley.
        story_progress = progress::ch7_resolve;
        break;

    case StoryId::ch7_east_gate:
        say_pages(ui, east_gate_ch7_pages);
        result.step_back = true;
        break;

    case StoryId::ch7_pass:
        say_pages(ui, pass_pages);
        result.battle = &pass_battle;
        break;

    case StoryId::ch8_shipwright:
        if(story_progress < progress::ch8_ship_found)
        {
            say_pages(ui, shipwright_pages);
            story_progress = progress::ch8_ship_found;
        }
        else if(story_progress < progress::ch8_rallied)
        {
            say_pages(ui, shipwright_wait_pages);
        }
        else if(story_progress < progress::ch8_ship)
        {
            say_pages(ui, shipwright_launch_pages);
            story_progress = progress::ch8_ship;
        }
        else
        {
            say_pages(ui, shipwright_after_pages);
        }
        break;

    case StoryId::ch8_pier:
        say_pages(ui, pier_pages);
        result.step_back = true;
        break;

    case StoryId::ch8_general_3:
        say_pages(ui, general3_pages);
        result.battle = &general3_battle;
        break;

    case StoryId::ch8_general_4:
        say_pages(ui, general4_pages);
        result.battle = &general4_battle;
        break;

    case StoryId::ch8_general_2:
        say_pages(ui, general2_last_pages);
        result.battle = &general2_last_battle;
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
    case StoryId::ch5_ruins:        // The partner's past.
    case StoryId::ch6_burning:      // The capital falls.
    case StoryId::ch6_aftermath:
    case StoryId::ch10_grave:       // The temptation of the ember.
        return BgmId::event_dark;

    case StoryId::ch7_shrine:       // The lantern is lit: the party is whole again.
    case StoryId::ch10_homecoming:
    case StoryId::ch10_elder:       // The coronation and the ending.
    case StoryId::ch10_seasons:
        return BgmId::event_hope;

    default:
        return BgmId::none;
    }
}

bool story_defeat_continues(StoryId id)
{
    return id == StoryId::ch6_general;
}

StoryResult run_story_after_battle(StoryId id, bool won, GameState& state, Ui& ui)
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

    case StoryId::ch5_cathedral:
        say_pages(ui, cathedral_victory_pages);
        state.story_progress = progress::ch6_start;
        break;

    case StoryId::ch7_pass:
        say_pages(ui, pass_victory_pages);
        state.story_progress = progress::ch8_start;
        break;

    case StoryId::ch8_general_3:
        say_pages(ui, general3_victory_pages);
        state.story_progress = progress::ch8_general_3;
        break;

    case StoryId::ch8_general_4:
        say_pages(ui, general4_victory_pages);
        state.story_progress = progress::ch8_general_4;
        break;

    case StoryId::ch8_general_2:
        say_pages(ui, general2_last_victory_pages);
        state.story_progress = progress::ch9_start;
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

    case StoryId::ch6_general:
        // Won or lost (it is meant to be lost), the capital falls and the party flees to the mining town.
        say_pages(ui, won ? bn::span<const bn::string_view>(rout_won_pages) :
                            bn::span<const bn::string_view>(rout_lost_pages));
        say_pages(ui, rout_pages);
        ui.fade_out(60);
        ui.wait(60);
        state.story_progress = progress::ch6_rout;
        state.party.restore_all();
        state.return_to_start();
        result.reload_map = true;
        result.next = StoryId::ch6_aftermath;
        break;

    default:
        break;
    }

    return result;
}

}
