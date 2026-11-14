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

#include "rpg_audio.h"
#include "rpg_enemies.h"

namespace rpg
{

class Ui;
class MessageBox;
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

    // Chapter 5 "Ashes of Betrayal" (40-49). The values are the SP column of the design document (E501-E507).
    constexpr uint8_t ch5_start = 40;       //!< E406: the wyrm of the Searing Canyon was defeated; on to the mining town.
    constexpr uint8_t ch5_dorn = 41;        //!< E501: saw the mining town that lives by obeying the cult.
    constexpr uint8_t ch5_deserter = 42;    //!< E502: saved the defector from the cult soldiers; he comes along as a guest.
    constexpr uint8_t ch5_mine = 44;        //!< E503: boss of the abandoned mine beaten; the defector joined for good.
    constexpr uint8_t ch5_kazami = 45;      //!< E504: the ruins of the partner's home village.
    constexpr uint8_t ch5_split = 46;       //!< E505: the defector's confession; the partner left the party.
    constexpr uint8_t ch5_cathedral = 48;   //!< E506: facing General Zelma in the Ash Cathedral (the partner returns in battle).

    // Chapter 6 "The Sinking Capital" (50-59), E601-E605.
    constexpr uint8_t ch6_start = 50;       //!< E507: Zelma fled; the next target is the capital.
    constexpr uint8_t ch6_audience = 52;    //!< E601: audience with the king: the party joins the defense.
    constexpr uint8_t ch6_eve = 53;         //!< E602: the night before the battle (the companions can be talked to).
    constexpr uint8_t ch6_gate = 55;        //!< E603: the gate was held through three battles in a row.
    constexpr uint8_t ch6_throne = 57;      //!< E604: lost to General Bardos in the throne room; the king was killed.
    constexpr uint8_t ch6_collapse = 59;    //!< E605: the hero's rage, the castle collapses, the party is scattered.

    // Chapter 7 "The Lamplight" (60-69), E701-E705.
    constexpr uint8_t ch7_start = 60;       //!< (The chapter really starts at 61: E605 leaves the progress at 59.)
    constexpr uint8_t ch7_awake = 61;       //!< E701: the hero woke up alone in the village of the refugees.
    constexpr uint8_t ch7_seer = 62;        //!< E702: the seer rejoined (the infirmary of the village).
    constexpr uint8_t ch7_defector = 63;    //!< E702: the defector rejoined (the broken bridge of the highway).
    constexpr uint8_t ch7_partner = 65;     //!< E702: the partner rejoined (the entrance of the marsh fort).
    constexpr uint8_t ch7_vow = 66;         //!< E703: the vow by the campfire.
    constexpr uint8_t ch7_bardos = 68;      //!< E704: General Bardos was killed: the first Ash General to fall.

    // Chapter 8 "Counterattack" (70-79), E801-E805. The two generals (E803, E804) can be beaten in any order:
    // they are tracked with FlagId::ryuze_defeated and FlagId::drake_defeated, not with the progress.
    constexpr uint8_t ch8_start = 70;       //!< E705: the signal fires were lit.
    constexpr uint8_t ch8_goals = 71;       //!< E801: the elder named the three goals: a ship and two generals.
    constexpr uint8_t ch8_ship = 73;        //!< E802: got the compass in the Sunken Lighthouse: the ship can sail.

    // Chapter 9 "The Flame Emperor's Castle" (80-89), E901-E905.
    constexpr uint8_t ch9_start = 80;       //!< E805: the send-off. The ship can reach the Emperor's island.
    constexpr uint8_t ch9_landing = 81;     //!< E901: landed on the island: there is no way back.
    constexpr uint8_t ch9_zelma = 84;       //!< E902: General Zelma fell in the castle (the partner and the defector).
    constexpr uint8_t ch9_motive = 86;      //!< E903: the Emperor told why he burns the world. The altar opens.
    constexpr uint8_t ch9_final = 88;       //!< E904: the Emperor was beaten once; the flame wearing his crown is left.

    // Chapter 10 "Green in the Ash" (90-99)
    constexpr uint8_t ch10_start = 90;      //!< The Emperor fell and his fire went out.
    constexpr uint8_t ch10_home = 91;       //!< Back in the home village, with the last ember of the pyre.
    constexpr uint8_t ch10_ember_out = 92;  //!< The hero almost fed the ember, then put it out.
    constexpr uint8_t ch10_rebuilt = 93;    //!< The village was rebuilt (another state of its map).

    constexpr uint8_t game_clear = 100;

    // After the ending, part 1 "The Runaway Necklace" (100-109). story_progress is a u8: there is room up to 255.
    constexpr uint8_t post_stranger = 101;      //!< Met and calmed the stranger whose necklace slips through time.
    constexpr uint8_t post_seer_elder = 102;    //!< The elder of the seers' village took on the reading of the necklace.
    constexpr uint8_t post_part_1_clear = 110;  //!< Where part 1 is meant to end (not reached yet).
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
    ch5_dorn,           //!< E501: arriving in the mining town.
    ch5_deserter,       //!< E502: behind the tavern: an event battle to save the defector.
    ch5_mine_gate,      //!< The abandoned mine, closed until the defector shows the way.
    ch5_overseer,       //!< E503: boss of the abandoned mine. The defector joins for good.
    ch5_kazami,         //!< E504: entering the ruins of the partner's village.
    ch5_confession,     //!< E505: the grave marker: the confession, a choice, and the partner leaves.
    ch5_cathedral_gate, //!< The way from the ruins to the cathedral, closed until the confession.
    ch5_east_gate,      //!< The main road to the cathedral: always shut.
    ch5_zelma,          //!< E506/E507: boss of the Ash Cathedral. The partner returns on the third round.
    ch6_audience,       //!< E601: entering the capital: audience with the king.
    ch6_inn,            //!< E602/E603: the inn of the capital: the eve, then three battles in a row at the gate.
    ch6_eve,            //!< E602: night falls (follows ch6_inn).
    ch6_eve_partner,    //!< E602: the three companions, on the eve.
    ch6_eve_seer,
    ch6_eve_defector,
    ch6_gate_2,         //!< E603: second and third battles at the gate (they follow ch6_inn).
    ch6_gate_3,
    ch6_palace,         //!< The palace doors: the way into the royal castle once the gate is held.
    ch6_great_gate,     //!< The great gate of the capital, barred for the siege.
    ch6_bardos,         //!< E604/E605: General Bardos in the throne room: a battle that is lost. The castle falls.
    ch6_road_lost,      //!< The road to the capital after it fell.
    ch7_awake,          //!< E701: waking up alone, saved by the child of the ash town (follows ch6_bardos).
    ch7_rico,           //!< The child, afterwards.
    ch7_seer,           //!< E702: the seer rejoins.
    ch7_south_gate,     //!< The track from the mining town to the village of the refugees, closed until chapter 7.
    ch7_keeper,         //!< The elder of the refugees (and, in chapter 8, the call to crew the ship).
    ch7_east_gate,      //!< The road east of the village, closed while the hero is alone.
    ch7_defector,       //!< E702: the defector rejoins at the broken bridge.
    ch7_partner,        //!< E702: the partner rejoins at the entrance of the marsh fort.
    ch7_fort_gate,      //!< The inner gate of the fort, closed until the vow.
    ch7_campfire,       //!< E703: the vow by the campfire.
    ch7_bardos,         //!< E704: boss of the marsh fort. Winning leads to ch7_signal.
    ch7_signal,         //!< E705: the signal fires. Starts chapter 8.
    ch8_lighthouse_gate,//!< The causeway to the Sunken Lighthouse, closed until the elder names the goals.
    ch8_lighthouse,     //!< E802: boss of the Sunken Lighthouse. Winning gives the compass.
    ch8_shipwright,     //!< The shipwright of the port town.
    ch8_pier,           //!< The pier: once the ship can sail, where to (the ice tower or the volcano fort).
    ch8_ryuze,          //!< E803: General Ryuze in the Tower of the Ice Wall.
    ch8_drake,          //!< E804: General Drake in the Volcano Fort.
    ch8_sendoff,        //!< E805: back in the village with both generals dead. Starts chapter 9.
    ch9_landing,        //!< E901: the island (follows the pier of the port town, see ch8_pier).
    ch9_no_return,      //!< The shore of the island: no way back.
    ch9_zelma,          //!< E902: General Zelma in the castle.
    ch9_motive,         //!< E903: the Emperor's room.
    ch9_altar,          //!< E904: the Altar of Rebirth: the Emperor, then (with no rest) ch9_flame.
    ch9_flame,          //!< E904: the Flame of Rebirth wearing his crown. E905 follows the victory.
    ch10_voyage,        //!< The pier of the harbor after the Emperor's fall: the ship sails home.
    ch10_homecoming,    //!< Arriving in the burnt home village (follows ch10_voyage).
    ch10_grave,         //!< At the parents' graves: the temptation of the ember.
    ch10_elder,         //!< The elder: rebuilding the village, then the coronation and the ending.
    ch10_seasons,       //!< The village after it is rebuilt (follows ch10_elder).
    post_stranger,      //!< After the ending: the stranger with the runaway necklace, in the home village.
    post_seer_elder,    //!< The elder of the seers' village: the first clue about the necklace.
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
    uint8_t battle_turn_limit = 0;          //!< The battle stops after this many rounds (0 = fought to the end).
    uint8_t battle_join_turn = 0;           //!< story_battle_reinforce() is called at the start of this round (0 = no).
};

/// Runs a scene (blocks until it ends). It may change the story progress, the party and the player position.
[[nodiscard]] StoryResult run_story(StoryId id, GameState& state, Ui& ui);

/**
 * Music of an important scene, played while it runs (FieldScene goes back to the map's music afterwards).
 * BgmId::none for the scenes that keep the music of the map.
 */
[[nodiscard]] BgmId story_bgm(StoryId id);

/**
 * Returns true if losing the boss battle of this scene is part of the story: instead of the usual penalty
 * (half the gold, back to town), the party is restored and run_story_after_battle() tells what happens.
 */
[[nodiscard]] bool story_defeat_continues(StoryId id);

/**
 * Called by the battle at the start of the round StoryResult::battle_join_turn: someone joins the fight.
 * Returns true if the party changed (the battle then refreshes its status window).
 */
[[nodiscard]] bool story_battle_reinforce(GameState& state, MessageBox& box);

/**
 * Runs what follows the boss battle started by a scene.
 * @param won false only for the scenes of story_defeat_continues() (other defeats never get here).
 */
[[nodiscard]] StoryResult run_story_after_battle(StoryId id, bool won, GameState& state, Ui& ui);

}

#endif
