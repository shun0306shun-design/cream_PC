/*
 * UI toolkit: window frames on a dynamic background + sprite text + blocking menus.
 *
 * Layout units:
 *   - Window positions and sizes are in 8x8 "cells" (the screen is 30x20 cells).
 *   - Text is placed in "lines" of `line_height` pixels inside a window.
 *
 * All the widgets are RAII: destroying a Window erases its frame and text.
 * Windows must not overlap, as erasing one would cut a hole in the other.
 *
 * Text is made of sprites (4-6 characters per sprite). The GBA shows up to 128 sprites,
 * so keep an eye on how much text is on screen at once.
 */

#ifndef RPG_UI_H
#define RPG_UI_H

#include "bn_span.h"
#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "bn_string_view.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_item.h"
#include "bn_sprite_text_generator.h"

#include "rpg_common.h"

namespace rpg
{

constexpr int screen_columns = screen_width / 8;
constexpr int screen_rows = screen_height / 8;
constexpr int line_height = 12;             //!< Pixels between text lines.
constexpr int max_window_sprites = 48;      //!< Text sprites per window.

// Background/sprite priorities (0 = front). The UI is always drawn over the scenes.
constexpr int ui_bg_priority = 0;
constexpr int scene_sprite_bg_priority = 2;
constexpr int scene_bg_priority = 3;

/**
 * Owns the window background layer and the text generator.
 * Create only one, after bn::core::init(), and share it with all the scenes.
 */
class Ui
{

public:
    Ui();

    Ui(const Ui& other) = delete;

    Ui& operator=(const Ui& other) = delete;

    /// Draws a window frame. Coordinates are in cells.
    void draw_frame(int col, int row, int cols, int rows);

    /// Makes a cells rectangle transparent again.
    void clear_cells(int col, int row, int cols, int rows);

    [[nodiscard]] const bn::sprite_text_generator& text_generator() const
    {
        return _text_generator;
    }

    /// Fixed width font (8 pixels per character): for text that must line up in columns, like the minimap.
    [[nodiscard]] const bn::sprite_text_generator& fixed_text_generator() const
    {
        return _fixed_text_generator;
    }

    /// Uploads pending window changes and waits for the next frame. Call this instead of bn::core::update().
    void update();

    void wait(int frames);

    /// Waits until A is pressed.
    void wait_for_a();

    /// Like wait_for_a(), but it also returns after the given frames or if B is pressed.
    void wait_for_a_or(int frames);

    /// Fades the whole screen to/from black.
    void fade_out(int frames = 16);
    void fade_in(int frames = 16);

private:
    bn::sprite_text_generator _text_generator;
    bn::sprite_text_generator _fixed_text_generator;
    bn::regular_bg_map_item _map_item;
    bn::regular_bg_ptr _bg;
    bn::regular_bg_map_ptr _map;
    bool _dirty = false;

    void _set_cell(int col, int row, int tile_index);
};


/// A framed box with text lines.
class Window
{

public:
    /// Window height in cells needed to show the given number of text lines.
    [[nodiscard]] static constexpr int rows_for_lines(int lines)
    {
        return ((lines * line_height) + 14 + 7) / 8;
    }

    Window(Ui& ui, int col, int row, int cols, int rows);

    Window(const Window& other) = delete;

    Window& operator=(const Window& other) = delete;

    ~Window();

    [[nodiscard]] Ui& ui() const
    {
        return _ui;
    }

    /// Prints one line of text. `x_offset` is in pixels from the left margin.
    void print(int line, const bn::string_view& text, int x_offset = 0);

    /// Like print(), but with the fixed width font.
    void print_fixed(int line, const bn::string_view& text, int x_offset = 0);

    /// Prints one line of text, horizontally centered in the window.
    void print_centered(int line, const bn::string_view& text);

    /// Prints text starting at `first_line`. '\n' starts a new line.
    void print_text(int first_line, const bn::string_view& text);

    void clear_text();

protected:
    Ui& _ui;
    int _col;
    int _row;
    int _cols;
    int _rows;
    bn::vector<bn::sprite_ptr, max_window_sprites> _sprites;

    /// Screen position in pixels (top-left origin) of a text line.
    [[nodiscard]] int _text_x() const
    {
        return (_col * 8) + 8;
    }

    [[nodiscard]] int _text_y(int line) const
    {
        return (_row * 8) + 5 + (line * line_height);
    }
};


/// A vertical list of options with a cursor. Scrolls if there are more options than visible lines.
class Menu : public Window
{

public:
    /**
     * @param options Must outlive the menu.
     * @param max_visible Maximum number of options shown at once.
     */
    Menu(Ui& ui, int col, int row, int cols, const bn::span<const bn::string_view>& options, int max_visible = 4);

    /**
     * Lets the player choose an option (blocks until then).
     * @return Index of the chosen option, or -1 if it was cancelled with B.
     */
    [[nodiscard]] int run(bool cancellable = true);

    /// Selects an option without user input.
    void set_index(int index);

private:
    bn::span<const bn::string_view> _options;
    int _visible;
    int _index = 0;
    int _top = 0;
    bn::sprite_ptr _cursor;

    void _redraw();
    void _update_cursor();
};


/// The classic message window at the bottom of the screen (4 lines).
class MessageBox : public Window
{

public:
    static constexpr int box_rows = rows_for_lines(4);

    explicit MessageBox(Ui& ui);

    /// Replaces the text without waiting.
    void show(const bn::string_view& text);

    /// Shows the text and waits for A.
    void say(const bn::string_view& text);

    /// Shows the text and waits for some frames (A or B skip the wait): battle pacing.
    void say_timed(const bn::string_view& text, int frames = 50);

    /// Shows several pages, waiting for A after each one.
    void say_pages(const bn::span<const bn::string_view>& pages);
};

}

#endif
