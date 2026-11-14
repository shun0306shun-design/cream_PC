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
#include "bn_vector.h"
#include "bn_bg_palettes.h"

#include "rpg_ui.h"
#include "rpg_text.h"
#include "rpg_audio.h"
#include "rpg_game_state.h"
#include "rpg_field_scene.h"
#include "rpg_battle_scene.h"

namespace
{
    /// Title screen: new game / continue / language.
    void run_title(rpg::GameState& state, rpg::Ui& ui)
    {
        enum class Option
        {
            new_game,
            continue_game,
            language
        };

        // "Continue" is only offered if there's a saved game in SRAM.
        bool has_save = rpg::has_saved_game();
        bool language_chosen = false;

        // Keeps playing until the first map chooses its own music.
        rpg::audio::play_bgm(rpg::BgmId::title);

        while(true)
        {
            // The language entry shows the current language. It is not saved: Japanese is the default
            // (rpg::default_language, rpg_text.h).
            bn::vector<bn::string_view, 3> labels;
            bn::vector<Option, 3> options;
            labels.push_back(RPG_T("New Game", "はじめから"));
            options.push_back(Option::new_game);

            if(has_save)
            {
                labels.push_back(RPG_T("Continue", "つづきから"));
                options.push_back(Option::continue_game);
            }

            labels.push_back(RPG_T("Language: English", "Language: 日本語"));
            options.push_back(Option::language);

            rpg::Window title(ui, 3, 3, 24, rpg::Window::rows_for_lines(2));
            title.print_centered(0, "BUTANO RPG FRAMEWORK");
            title.print_centered(1, RPG_T("Turn-based battle demo", "ターン制バトル デモ"));

            rpg::Menu menu(ui, 6, 11, 17, bn::span<const bn::string_view>(labels.data(), labels.size()));

            if(language_chosen)
            {
                menu.set_index(labels.size() - 1);
            }
            else if(has_save)
            {
                menu.set_index(1);
            }

            Option option = options[menu.run(false)];

            if(option == Option::language)
            {
                // Switch and draw the title again (the windows are erased at the end of this block).
                bool english = rpg::language() == rpg::Language::english;
                rpg::set_language(english ? rpg::Language::japanese : rpg::Language::english);
                language_chosen = true;
                continue;
            }

            if(option == Option::new_game || ! rpg::load_game(state))
            {
                state.init_new_game();
            }

            ui.fade_out();      // With the title still on screen.
            break;
        }
    }
}

int main()
{
    bn::core::init();
    rpg::audio::init();

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

        state.story_battle_lost = false;

        if(result == rpg::BattleResult::defeat && rpg::story_defeat_continues(state.story_battle))
        {
            // A battle the story expects to be lost: no penalty, the next FieldScene shows what follows.
            state.party.restore_all();
            state.story_battle_lost = true;
            continue;
        }

        if(result != rpg::BattleResult::victory)
        {
            // Lost a story battle: the scene that started it can be tried again.
            // After a victory, the next FieldScene shows what follows (rpg::run_story_after_battle).
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
