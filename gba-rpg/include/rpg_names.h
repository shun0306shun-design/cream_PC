/*
 * Placeholder proper names of the story.
 *
 * Every name the script mentions is defined once here, as a string literal macro, so dialogue can be written as
 * RPG_NAME_HERO ": ..." and still be a compile time constant stored in ROM. To rename someone, edit this file.
 * (If the hero's name becomes player input, replace the uses of RPG_NAME_HERO with run time formatting.)
 */

#ifndef RPG_NAMES_H
#define RPG_NAMES_H

#define RPG_NAME_HERO       "Allen"     // The hero (to be chosen by the player).
#define RPG_NAME_PARTNER    "Sion"      // The partner: a bounty hunter who fights with a spear.
#define RPG_NAME_VILLAGE    "Hart"      // T01: the hero's home village.
#define RPG_NAME_TOWN       "Berg"      // T02: the post town.
#define RPG_NAME_CULT       "Ashen Cult"
#define RPG_NAME_EMPEROR    "Valgren"   // The Flame Emperor, leader of the cult.
#define RPG_NAME_GENERAL_1  "Drake"     // The first of the four Ash Generals.

#endif
