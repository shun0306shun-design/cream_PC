/*
 * Main story: progress values and scripted events (cutscenes, boss battles, party joins).
 *
 * The whole main story is tracked by one number, GameState::story_progress (saved):
 *   chapter 1 = 0-9, chapter 2 = 10-19, ... 10 per chapter, 100 = game cleared.
 * Bit flags (rpg_flags.h) are only for optional things like chests and side events.
 *
 * Map events appear and disappear with the progress (MapEvent::min_progress / max_progress), and the lines of
 * ordinary NPCs change with the chapter (MapEvent::chapter_pages). Scripted scenes are map events of type
 * EventType::story (press A) or EventType::trigger (step on the tile) that name a StoryId handled in rpg_story.cpp.
 *
 * To add a scene: add a StoryId, write its script in rpg_story.cpp and place an event with it in rpg_maps.cpp.
 */

#ifndef RPG_STORY_H
#define RPG_STORY_H

#include "rpg_enemies.h"

namespace rpg
{

class Ui;
struct GameState;

/// Milestones of GameState::story_progress. A value means "this already happened".
namespace progress
{
    // Chapter 1 "Morning of Ash" (0-9)
    constexpr uint8_t ch1_start = 0;        //!< New game: the opening hasn't been shown yet.
    constexpr uint8_t ch1_morning = 1;      //!< Peaceful village, free to walk around.
    constexpr uint8_t ch1_errand = 2;       //!< Father asked for water from the spring.
    constexpr uint8_t ch1_ashes = 3;        //!< The village burned down. The parents are dead.
    constexpr uint8_t ch1_departed = 4;     //!< The elder pointed to the Watch Cave: the journey starts.

    // Chapter 2 "Blades That Never Cross" (10-19)
    constexpr uint8_t ch2_start = 10;       //!< The cult scout of the Watch Cave was defeated (crest found).
    constexpr uint8_t ch2_rivals = 11;      //!< Quarrel with the partner in the post town.
    constexpr uint8_t ch2_allied = 12;      //!< Fighting General 1 side by side (the partner is in the party).

    // Chapter 3 "The Star-Reader Girl" (20-29)
    constexpr uint8_t ch3_start = 20;       //!< General 1 was driven off: the partner joined for good.
    constexpr uint8_t ch3_seer = 21;        //!< The seer joined in her village: the Star Ruins open.
    constexpr uint8_t ch3_sigil = 22;       //!< Star Ruins cleared: got the Star Sigil that opens the waterway.

    // Chapter 4 "The Scorched Land" (30-39)
    constexpr uint8_t ch4_start = 30;       //!< Crossed under the gate through the waterway.
    constexpr uint8_t ch4_wasteland = 31;   //!< Saw the burnt plains beyond the gate.
    constexpr uint8_t ch4_truth = 32;       //!< Learned that eight parts in ten of the world burned: the canyon opens.

    // Chapter 5 "Ashes of Betrayal" (40-49)
    constexpr uint8_t ch5_start = 40;       //!< The wyrm of the Searing Canyon was defeated.
    constexpr uint8_t ch5_defector = 41;    //!< The former cult priest joined: the ruins of the partner's village open.
    constexpr uint8_t ch5_split = 42;       //!< The partner's past came out and he left the party: the cathedral opens.
    constexpr uint8_t ch5_reunited = 43;    //!< The partner came back for the battle against General 1.

    // Chapter 6 "The Sinking Capital" (50-59)
    constexpr uint8_t ch6_start = 50;       //!< General 1 was killed in the Ash Cathedral: the road to the capital opens.
    constexpr uint8_t ch6_muster = 51;      //!< The captain asked for help on the east wall.
    constexpr uint8_t ch6_siege = 52;       //!< The wall fell: the capital burns (another state of its map).
    constexpr uint8_t ch6_rout = 53;        //!< The hero broke the line, lost to General 2 and the party fled.

    // Chapter 7 (60-69)
    constexpr uint8_t ch7_start = 60;       //!< The capital is lost. The party is back in the mining town, broken.

    constexpr uint8_t game_clear = 100;
}

/// Chapter number counting from 0 (0 = chapter 1): what NPC lines are selected with.
[[nodiscard]] constexpr int chapter_index(int story_progress)
{
    return story_progress / 10;
}

enum class StoryId : uint8_t
{
    ch1_opening,        //!< Shown when a new game starts.
    ch1_father,         //!< Father gives the errand.
    ch1_attack,         //!< At the spring: the cult burns the village.
    ch1_awakening,      //!< Waking up in the ruins (follows ch1_attack).
    ch1_gate,           //!< The village gate, before the journey can start.
    ch1_elder,          //!< The elder sends the hero to the Watch Cave.
    ch1_scout,          //!< Boss of the Watch Cave. Winning starts chapter 2.
    ch2_road,           //!< The road to the post town, closed during chapter 1.
    ch2_partner_town,   //!< First meeting with the partner.
    ch2_east_gate,      //!< The highway to the fort, closed until the quarrel.
    ch2_fort,           //!< Boss of the ruined fort, fought together. Winning starts chapter 3.
    ch3_seer_road,      //!< The mountain path to the seer's village, closed until chapter 3.
    ch3_seer_village,   //!< Meeting the seer: she joins.
    ch3_ruins_gate,     //!< The way to the Star Ruins, closed until the seer joins.
    ch3_altar,          //!< Boss of the Star Ruins. Winning gives the Star Sigil.
    ch3_sluice,         //!< The sealed entrance of the waterway (needs the Star Sigil).
    ch3_warden,         //!< Boss of the waterway. Winning starts chapter 4.
    ch4_wasteland,      //!< First steps beyond the gate: nothing but ash.
    ch4_chronicler,     //!< The chronicler of the ash town shows the map of what burned.
    ch4_canyon_gate,    //!< The way to the Searing Canyon, closed until the truth is known.
    ch4_wyrm,           //!< Boss of the Searing Canyon. Winning starts chapter 5.
    ch5_defector,       //!< Meeting the former cult priest in the mining town: he joins.
    ch5_north_gate,     //!< The road to the ruined village, closed until he joins.
    ch5_ruins,          //!< At the grave of the ruined village: the partner's past. He leaves the party.
    ch5_east_gate,      //!< The way to the Ash Cathedral, closed until the party splits.
    ch5_cathedral,      //!< Boss of the Ash Cathedral: the partner returns. Winning starts chapter 6.
    ch6_captain,        //!< The captain of the capital asks for help.
    ch6_east_gate,      //!< The east gate of the capital: going there starts the siege.
    ch6_burning,        //!< The capital burns (follows ch6_east_gate).
    ch6_general,        //!< General 2 at the palace: a battle that is meant to be lost. Then the rout.
    ch6_aftermath,      //!< Back in the mining town after the rout. Starts chapter 7.
    ch6_road_lost,      //!< The road to the capital after it fell.
    count,
    none = 0xFF
};

/// What FieldScene must do after a scene.
struct StoryResult
{
    const EncounterGroup* battle = nullptr; //!< Boss battle to start now (it can't be escaped), or null.
    bool step_back = false;                 //!< Push the player back to the tile it came from (closed ways).
    bool reload_map = false;                //!< The scene ended with the screen faded out: reload the map and fade in.
    StoryId next = StoryId::none;           //!< Scene to run right after this one.
};

/// Runs a scene (blocks until it ends). It may change the story progress, the party and the player position.
[[nodiscard]] StoryResult run_story(StoryId id, GameState& state, Ui& ui);

/**
 * Returns true if losing the boss battle of this scene is part of the story: instead of the usual penalty
 * (half the gold, back to town), the party is restored and run_story_after_battle() tells what happens.
 */
[[nodiscard]] bool story_defeat_continues(StoryId id);

/**
 * Runs what follows the boss battle started by a scene.
 * @param won false only for the scenes of story_defeat_continues() (other defeats never get here).
 */
[[nodiscard]] StoryResult run_story_after_battle(StoryId id, bool won, GameState& state, Ui& ui);

}

#endif
