/*
 * Classic turn based battle: the party chooses commands, then everyone acts ordered by agility.
 */

#ifndef RPG_BATTLE_SCENE_H
#define RPG_BATTLE_SCENE_H

#include "bn_string.h"
#include "bn_vector.h"
#include "bn_sprite_ptr.h"

#include "rpg_ui.h"
#include "rpg_enemies.h"

namespace rpg
{

struct Party;
struct GameState;

enum class BattleResult : uint8_t
{
    victory,
    defeat,
    escaped
};

class BattleScene
{

public:
    /// @param can_escape false for story battles: the "Run" command always fails.
    BattleScene(GameState& state, Ui& ui, const EncounterGroup& group, bool can_escape = true);

    /// Runs the whole battle (blocks until it ends).
    [[nodiscard]] BattleResult run();

private:
    struct Enemy
    {
        const EnemyData* data;
        int16_t hp;
        bn::string<32> name;        //!< With a letter suffix if there are several of the same kind.
        bn::sprite_ptr sprite;

        [[nodiscard]] bool alive() const
        {
            return hp > 0;
        }
    };

    enum class ActionType : uint8_t
    {
        none,
        attack,
        spell,
        item
    };

    /// What a party member will do this round.
    struct Action
    {
        ActionType type = ActionType::none;
        uint8_t id = 0;         //!< SpellId or ItemId, depending on the type.
        int8_t target = -1;     //!< Enemy or party member index, -1 for everyone.
    };

    struct Turn
    {
        bool is_enemy;
        int8_t index;
        int16_t initiative;
    };

    enum class CommandResult : uint8_t
    {
        confirmed,
        back,
        run
    };

    GameState& _state;
    Ui& _ui;
    Party& _party;
    bn::vector<Enemy, max_enemies> _enemies;
    Window _status_window;
    Action _actions[max_party_size];
    bool _can_escape;

    [[nodiscard]] BattleResult _run_battle();
    void _refresh_status();

    // --- Command selection ---
    [[nodiscard]] bool _select_commands();
    [[nodiscard]] CommandResult _select_command(int member_index);
    [[nodiscard]] bool _choose_spell(int member_index, Action& action);
    [[nodiscard]] bool _choose_item(Action& action);
    [[nodiscard]] bool _choose_target(const Effect& effect, Action& action);
    [[nodiscard]] int _choose_enemy();
    [[nodiscard]] int _choose_ally();
    void _notice(const bn::string_view& text);

    // --- Round execution ---
    void _execute_round(MessageBox& box);
    void _party_turn(int member_index, MessageBox& box);
    void _enemy_turn(int enemy_index, MessageBox& box);
    void _apply_party_effect(const Effect& effect, int target, MessageBox& box);
    void _apply_enemy_effect(const Effect& effect, int enemy_index, MessageBox& box);
    void _damage_enemy(int enemy_index, int amount, MessageBox& box);
    void _damage_member(int member_index, int amount, MessageBox& box);
    void _heal_member(int member_index, const Effect& effect, MessageBox& box);
    void _heal_enemy(int enemy_index, int amount, MessageBox& box);

    [[nodiscard]] bool _try_escape(MessageBox& box);
    void _victory(MessageBox& box);

    // --- Helpers ---
    [[nodiscard]] bool _all_enemies_dead() const;
    [[nodiscard]] int _valid_enemy_target(int enemy_index) const;
    [[nodiscard]] int _random_alive_member() const;
};

}

#endif
