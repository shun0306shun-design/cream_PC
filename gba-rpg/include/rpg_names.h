/*
 * Placeholder proper names of the story.
 *
 * Every name the script mentions is defined once here, as a string literal macro, so dialogue can be written as
 * RPG_NAME_HERO ": ..." and still be a compile time constant stored in ROM. To rename someone, edit this file.
 * (If the hero's name becomes player input, replace the uses of RPG_NAME_HERO with run time formatting.)
 *
 * Each name has an English and a Japanese (_JA) version, used by the two halves of RPG_T texts (rpg_text.h).
 */

#ifndef RPG_NAMES_H
#define RPG_NAMES_H

#define RPG_NAME_HERO       "Allen"     // The hero (to be chosen by the player).
#define RPG_NAME_PARTNER    "Sion"      // The partner: a bounty hunter who fights with a spear.
#define RPG_NAME_VILLAGE    "Hart"      // T01: the hero's home village.
#define RPG_NAME_TOWN       "Berg"      // T02: the post town.
#define RPG_NAME_SEER       "Mirea"     // The star-reader girl: joins in chapter 3 (the mage class).
#define RPG_NAME_SEER_TOWN  "Lunaris"   // T03: the village of fortune tellers.
#define RPG_NAME_GATE       "Great Gate" // The checkpoint that closes the way north.
#define RPG_NAME_ASH_TOWN   "Ashbell"   // T04: the town of survivors beyond the gate.
#define RPG_NAME_DEFECTOR   "Nox"       // The former cult priest: joins in chapter 5 (the priest class).
#define RPG_NAME_MINE_TOWN  "Dorn"      // T05: the mining town.
#define RPG_NAME_PARTNER_HOME "Kazami"  // The partner's home village, burnt long ago.
#define RPG_NAME_CAPITAL    "Grancel"   // T06: the royal capital, the last city standing.
#define RPG_NAME_GENERAL_2  "Vesna"     // The second Ash General: takes the capital in chapter 6.
#define RPG_NAME_REFUGE     "Lanternrest" // T07: the hidden valley of the refugees.
#define RPG_NAME_HARBOR     "Saltmere"  // T08: the harbor town.
#define RPG_NAME_SHIP       "Dawn"      // The last ship of the harbor.
#define RPG_NAME_GENERAL_3  "Garm"      // The third Ash General.
#define RPG_NAME_GENERAL_4  "Ilse"      // The fourth Ash General.
#define RPG_NAME_CULT       "Ashen Cult"
#define RPG_NAME_EMPEROR    "Valgren"   // The Flame Emperor, leader of the cult.
#define RPG_NAME_GENERAL_1  "Drake"     // The first of the four Ash Generals.

#define RPG_NAME_HERO_JA        "アレン"
#define RPG_NAME_PARTNER_JA     "シオン"
#define RPG_NAME_VILLAGE_JA     "ハルト"
#define RPG_NAME_TOWN_JA        "ベルク"
#define RPG_NAME_SEER_JA        "ミレア"
#define RPG_NAME_SEER_TOWN_JA   "ルナリス"
#define RPG_NAME_GATE_JA        "大関所"
#define RPG_NAME_ASH_TOWN_JA    "アッシュベル"
#define RPG_NAME_DEFECTOR_JA    "ノクス"
#define RPG_NAME_MINE_TOWN_JA   "ドルン"
#define RPG_NAME_PARTNER_HOME_JA "カザミ"
#define RPG_NAME_CAPITAL_JA     "グランセル"
#define RPG_NAME_REFUGE_JA      "ランタンレスト"
#define RPG_NAME_CULT_JA        "灰の教団"
#define RPG_NAME_EMPEROR_JA     "ヴァルグレン"
#define RPG_NAME_GENERAL_1_JA   "ドレイク"
#define RPG_NAME_GENERAL_2_JA   "ヴェスナ"

#endif
