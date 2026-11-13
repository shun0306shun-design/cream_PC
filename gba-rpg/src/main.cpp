/*
 * Butano RPG framework - entry point.
 *
 * Scene flow:   title -> field <-> battle
 *
 * Scenes are plain objects created on the stack, one at a time, so the graphics of a scene
 * (backgrounds, sprites, VRAM) are released before the next one starts.
 * Everything that must survive between scenes lives in rpg::GameState.
 */

#include "bn_core.h"
#include "bn_color.h"
#include "bn_bg_palettes.h"

#include "rpg_ui.h"
#include "rpg_game_state.h"
#include "rpg_field_scene.h"
#include "rpg_battle_scene.h"

namespace
{
    /// Title screen: new game / continue.
    void run_title(rpg::GameState& state, rpg::Ui& ui)
    {
        static constexpr bn::string_view options[] = { "New Game", "Continue" };

        rpg::Window title(ui, 3, 3, 24, rpg::Window::rows_for_lines(2));
        title.print_centered(0, "BUTANO RPG FRAMEWORK");
        title.print_centered(1, "Turn-based battle demo");

        // "Continue" is only offered if there's a saved game in SRAM.
        int options_count = rpg::has_saved_game() ? 2 : 1;
        rpg::Menu menu(ui, 9, 11, 12, bn::span<const bn::string_view>(options, options_count));

        if(options_count == 2)
        {
            menu.set_index(1);
        }

        if(menu.run(false) == 0 || ! rpg::load_game(state))
        {
            state.init_new_game();
        }

        ui.fade_out();
    }
}

int main()
{
    bn::core::init();

    // Color shown where no background is drawn (for example behind the battle screen).
    bn::bg_palettes::set_transparent_color(bn::color(0, 0, 0));

    rpg::Ui ui;
    rpg::GameState state;
    run_title(state, ui);

    while(true)
    {
        // Explore until a random encounter happens.
        const rpg::EncounterGroup* encounter;

        {
            rpg::FieldScene field(state, ui);
            encounter = &field.run();
        }

        // Fight.
        rpg::BattleResult result;

        {
            // Story battles (started by a scene, see rpg_story.h) can't be escaped.
            bool story_battle = state.story_battle != rpg::StoryId::none;
            rpg::BattleScene battle(state, ui, *encounter, ! story_battle);
            result = battle.run();
        }

        if(result != rpg::BattleResult::victory)
        {
            // Lost a story battle: the scene that started it can be tried again.
            // After a victory, the next FieldScene shows what follows (rpg::run_story_victory).
            state.story_battle = rpg::StoryId::none;
        }

        if(result == rpg::BattleResult::defeat)
        {
            // Classic penalty: wake up in the last safe town with half the gold.
            state.party.restore_all();
            state.party.gold /= 2;
            state.return_to_start();
        }
    }
}
