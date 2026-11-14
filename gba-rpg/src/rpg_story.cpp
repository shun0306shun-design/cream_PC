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
              "記録者「皇帝は征服しない。\n　火に、まきを\n　くべているだけさ。」"),
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
        "A man in a cult robe sits\nalone. The miners give\nhim a wide berth.",
        HERO ": That crest--!",
        "???: Put the sword down.\nI took the robe off once.\nIt doesn't come off.",
        NOX ": I am " NOX ". I was a\npriest of the " RPG_NAME_CULT ".\nI lit their fires.",
        PARTNER ": Then burn.",
        NOX ": Soon enough. First:\n" GENERAL " prays at the Ash\nCathedral, east of here.",
        NOX ": Its doors open only\nto the seal of a priest.\nI can open them.",
        NOX ": But I buried my seal\nin a ruin, north of here.\nWe need it first.",
        SEER ": His star is dim.\nBut it is not lying.",
        HERO ": ...You walk in\nfront. Where I can\nsee you.",
        NOX " joined the party!",
    };

    constexpr bn::string_view north_gate_pages[] = {
        "A road north, into ruins.\nThe miners say no one\nlives there anymore.",
    };

    constexpr bn::string_view east_gate_ch5_pages[] = {
        "A black spire rises in\nthe east. Its doors are\nsealed to outsiders.",
    };

    constexpr bn::string_view ruins_pages[] = {
        "A single grave marker.\nSomeone tended it, once.",
        PARTNER ": ...Stop.\nDon't touch that.",
        PARTNER ": This is " RPG_NAME_PARTNER_HOME ".\nThis was my home.",
        HERO ": You said you hunt\nfor bounties.",
        PARTNER ": A bounty is\nlighter to carry than\na grave.",
        NOX ": ...I know this\nplace. I held a torch\nhere, that night.",
        PARTNER ": You.",
        PARTNER "'s spear is at\n" NOX "'s throat. " HERO "\nsteps between them.",
        PARTNER ": You'd shield him?\nAfter " RPG_NAME_VILLAGE "? After your\nown dead?",
        HERO ": We need him to\nreach " GENERAL ".",
        PARTNER ": Then need him.\nI'm done needing anyone.",
        PARTNER " left the party.",
        NOX ": The seal is here.\n...So is everything\nelse I buried.",
    };

    constexpr bn::string_view cathedral_pages[] = {
        "The doors open at " NOX "'s\ntouch. " GENERAL " kneels\nbefore a pyre.",
        GENERAL ": The traitor, the\nstray, the stargazer. And\nno spear. Did he break?",
        HERO ": He's not here.\nI am.",
        "A spear strikes the floor\nbetween them.",
        PARTNER ": I didn't come\nfor you, " HERO ".",
        PARTNER ": " RPG_NAME_PARTNER_HOME " deserves\nbetter than a son who\nruns. Move over.",
        PARTNER " rejoined the party!",
    };

    constexpr bn::string_view cathedral_again_pages[] = {
        PARTNER ": Again.\nHe falls today.",
    };

    constexpr bn::string_view cathedral_victory_pages[] = {
        GENERAL ": So the ash\nremembers its dead...",
        GENERAL ": I gave the orders.\nBut the fire is his.\nIt will eat you, too.",
        GENERAL " falls.\nThe pyre goes out.",
        PARTNER ": ...It doesn't\nfeel like anything.",
        PARTNER ": " NOX ". I won't\nforgive you. But I'll\nstand next to you.",
        NOX ": That is more than\nI will ever ask.",
        HERO ": One general.\nThree left. Then him.",
        "- Chapter 6 -",
    };

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

    case StoryId::ch5_cathedral:
        say_pages(ui, cathedral_victory_pages);
        state.story_progress = progress::ch6_start;
        break;

    default:
        break;
    }
}

}
