#include "rpg_ui.h"

#include "bn_core.h"
#include "bn_color.h"
#include "bn_keypad.h"
#include "bn_memory.h"
#include "bn_bg_tiles.h"
#include "bn_bg_palettes.h"
#include "bn_sprite_palettes.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_cell_info.h"

#include "bn_sprite_items_cursor.h"
#include "bn_bg_palette_items_ui_palette.h"
#include "bn_regular_bg_tiles_items_ui_tiles.h"

#include "common_fixed_8x16_sprite_font.h"
#include "common_variable_8x16_sprite_font.h"
#include "rpg_japanese_sprite_font.h"
#include "rpg_audio.h"

namespace rpg
{

namespace
{
    // Tile indexes of graphics/ui_tiles.bmp.
    constexpr int tile_empty = 0;
    constexpr int tile_fill = 1;
    constexpr int tile_top_left = 2;
    constexpr int tile_top = 3;
    constexpr int tile_top_right = 4;
    constexpr int tile_left = 5;
    constexpr int tile_right = 6;
    constexpr int tile_bottom_left = 7;
    constexpr int tile_bottom = 8;
    constexpr int tile_bottom_right = 9;

    // The window layer is a 32x32 cells (256x256 pixels) background.
    // Its cells live in EWRAM instead of the (small) stack.
    constexpr int map_columns = 32;
    constexpr int map_rows = 32;

    alignas(int) BN_DATA_EWRAM_BSS bn::regular_bg_map_cell ui_cells[map_columns * map_rows];

    [[nodiscard]] bn::regular_bg_ptr create_bg(const bn::regular_bg_map_item& map_item)
    {
        bn::memory::clear(ui_cells);

        // Dynamic maps reference tiles by raw index, so the tiles must not be offset in VRAM.
        bn::bg_tiles::set_allow_offset(false);

        bn::regular_bg_item bg_item(
                    bn::regular_bg_tiles_items::ui_tiles, bn::bg_palette_items::ui_palette, map_item);

        // Backgrounds are centered on the screen: move it so cell (0, 0) is the top-left corner.
        bn::regular_bg_ptr bg = bg_item.create_bg(((map_columns * 8) - screen_width) / 2,
                                                  ((map_rows * 8) - screen_height) / 2);
        bg.set_priority(ui_bg_priority);

        bn::bg_tiles::set_allow_offset(true);
        return bg;
    }

    void set_fade(bn::fixed intensity)
    {
        bn::color black(0, 0, 0);
        bn::bg_palettes::set_fade(black, intensity);
        bn::sprite_palettes::set_fade(black, intensity);
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Ui
// ---------------------------------------------------------------------------------------------------------------------

Ui::Ui() :
    _text_generator(common::variable_8x16_sprite_font),
    _japanese_text_generator(japanese_sprite_font),
    _fixed_text_generator(common::fixed_8x16_sprite_font),
    _map_item(ui_cells[0], bn::size(map_columns, map_rows)),
    _bg(create_bg(_map_item)),
    _map(_bg.map())
{
    _text_generator.set_bg_priority(ui_bg_priority);
    _text_generator.set_left_alignment();
    _japanese_text_generator.set_bg_priority(ui_bg_priority);
    _japanese_text_generator.set_left_alignment();
    _fixed_text_generator.set_bg_priority(ui_bg_priority);
    _fixed_text_generator.set_left_alignment();
}

void Ui::draw_frame(int col, int row, int cols, int rows)
{
    BN_ASSERT(col >= 0 && row >= 0 && cols >= 2 && rows >= 2 &&
              col + cols <= screen_columns && row + rows <= screen_rows, "Invalid window frame");

    int right = col + cols - 1;
    int bottom = row + rows - 1;

    for(int y = row; y <= bottom; ++y)
    {
        for(int x = col; x <= right; ++x)
        {
            int tile_index = tile_fill;

            if(y == row)
            {
                tile_index = x == col ? tile_top_left : (x == right ? tile_top_right : tile_top);
            }
            else if(y == bottom)
            {
                tile_index = x == col ? tile_bottom_left : (x == right ? tile_bottom_right : tile_bottom);
            }
            else if(x == col)
            {
                tile_index = tile_left;
            }
            else if(x == right)
            {
                tile_index = tile_right;
            }

            _set_cell(x, y, tile_index);
        }
    }
}

void Ui::clear_cells(int col, int row, int cols, int rows)
{
    for(int y = row; y < row + rows; ++y)
    {
        for(int x = col; x < col + cols; ++x)
        {
            _set_cell(x, y, tile_empty);
        }
    }
}

void Ui::update()
{
    if(_dirty)
    {
        // Tell Butano that the cells have changed: they are copied to VRAM in the next V-Blank.
        _map.reload_cells_ref();
        _dirty = false;
    }

    bn::core::update();
}

void Ui::wait(int frames)
{
    for(int frame = 0; frame < frames; ++frame)
    {
        update();
    }
}

void Ui::wait_for_a()
{
    do
    {
        update();
    }
    while(! bn::keypad::a_pressed());
}

void Ui::wait_for_a_or(int frames)
{
    for(int frame = 0; frame < frames; ++frame)
    {
        update();

        if(bn::keypad::a_pressed() || bn::keypad::b_pressed())
        {
            return;
        }
    }
}

void Ui::fade_out(int frames)
{
    for(int frame = 1; frame <= frames; ++frame)
    {
        set_fade(bn::fixed(frame) / frames);
        update();
    }
}

void Ui::fade_in(int frames)
{
    for(int frame = frames - 1; frame >= 0; --frame)
    {
        set_fade(bn::fixed(frame) / frames);
        update();
    }
}

void Ui::_set_cell(int col, int row, int tile_index)
{
    bn::regular_bg_map_cell& cell = ui_cells[_map_item.cell_index(col, row)];
    bn::regular_bg_map_cell_info cell_info(cell);
    cell_info.set_tile_index(tile_index);
    cell = cell_info.cell();
    _dirty = true;
}

// ---------------------------------------------------------------------------------------------------------------------
// Window
// ---------------------------------------------------------------------------------------------------------------------

Window::Window(Ui& ui, int col, int row, int cols, int rows) :
    _ui(ui),
    _col(col),
    _row(row),
    _cols(cols),
    _rows(rows)
{
    _ui.draw_frame(col, row, cols, rows);
}

Window::~Window()
{
    _ui.clear_cells(_col, _row, _cols, _rows);
}

void Window::print(int line, const bn::string_view& text, int x_offset)
{
    // The "top_left" methods of Butano use screen coordinates with the origin in the top-left corner.
    _ui.text_generator().generate_top_left(_text_x() + x_offset, _text_y(line), localized(text), _sprites);
}

void Window::print_fixed(int line, const bn::string_view& text, int x_offset)
{
    _ui.fixed_text_generator().generate_top_left(_text_x() + x_offset, _text_y(line), text, _sprites);
}

void Window::print_centered(int line, const bn::string_view& text)
{
    bn::string_view line_text = localized(text);
    int inner_width = (_cols * 8) - 16;
    print(line, line_text, (inner_width - _ui.text_generator().width(line_text)) / 2);
}

void Window::print_text(int first_line, const bn::string_view& full_text, int line_pitch)
{
    // Choose the language first: a RPG_T text holds the lines of both.
    bn::string_view text = localized(full_text);
    int line = 0;
    int line_start = 0;
    int size = text.size();

    for(int index = 0; index <= size; ++index)
    {
        if(index == size || text[index] == '\n')
        {
            if(index > line_start)
            {
                _ui.text_generator().generate_top_left(_text_x(), _text_y(first_line) + (line * line_pitch),
                                                       text.substr(line_start, index - line_start), _sprites);
            }

            line_start = index + 1;
            ++line;
        }
    }
}

void Window::clear_text()
{
    _sprites.clear();
}

// ---------------------------------------------------------------------------------------------------------------------
// Menu
// ---------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr int cursor_width = 10;    // Pixels reserved for the cursor at the left of the options.

    [[nodiscard]] int visible_options(int options_count, int max_visible)
    {
        BN_ASSERT(options_count > 0, "A menu needs at least one option");

        return options_count < max_visible ? options_count : max_visible;
    }
}

Menu::Menu(Ui& ui, int col, int row, int cols, const bn::span<const bn::string_view>& options, int max_visible) :
    Window(ui, col, row, cols, rows_for_lines(visible_options(options.size(), max_visible))),
    _options(options),
    _visible(visible_options(options.size(), max_visible)),
    _cursor(bn::sprite_items::cursor.create_sprite(0, 0))
{
    _cursor.set_bg_priority(ui_bg_priority);
    _redraw();
}

int Menu::run(bool cancellable)
{
    int count = _options.size();

    while(true)
    {
        // Update first: the key press that opened this menu must not be read again.
        _ui.update();

        if(bn::keypad::a_pressed())
        {
            audio::play_se(SeId::confirm);
            return _index;
        }

        if(cancellable && bn::keypad::b_pressed())
        {
            audio::play_se(SeId::cancel);
            return -1;
        }

        if(bn::keypad::up_pressed())
        {
            set_index(_index > 0 ? _index - 1 : count - 1);
        }
        else if(bn::keypad::down_pressed())
        {
            set_index(_index < count - 1 ? _index + 1 : 0);
        }
    }
}

void Menu::set_index(int index)
{
    BN_ASSERT(index >= 0 && index < _options.size(), "Invalid menu index: ", index);

    _index = index;

    // Scroll to keep the cursor visible.
    if(_index < _top)
    {
        _top = _index;
        _redraw();
    }
    else if(_index >= _top + _visible)
    {
        _top = _index - _visible + 1;
        _redraw();
    }
    else
    {
        _update_cursor();
    }
}

void Menu::_redraw()
{
    clear_text();

    for(int line = 0; line < _visible; ++line)
    {
        print(line, _options[_top + line], cursor_width);
    }

    _update_cursor();
}

void Menu::_update_cursor()
{
    // Vertically centered in its text line (the cursor is 8x8 and the font is 16 pixels tall).
    _cursor.set_top_left_position(_text_x(), _text_y(_index - _top) + 4);
}

// ---------------------------------------------------------------------------------------------------------------------
// MessageBox
// ---------------------------------------------------------------------------------------------------------------------

MessageBox::MessageBox(Ui& ui) :
    Window(ui, 0, screen_rows - box_rows, screen_columns, box_rows)
{
}

void MessageBox::show(const bn::string_view& text)
{
    bn::string_view page = localized(text);
    int line_pitch = line_height;

    if(language() == Language::japanese)
    {
        int lines = 1;

        for(char character : page)
        {
            if(character == '\n')
            {
                ++lines;
            }
        }

        if(lines <= japanese_lines)
        {
            line_pitch = japanese_line_pitch;
        }
    }

    clear_text();
    print_text(0, page, line_pitch);
}

void MessageBox::say(const bn::string_view& text)
{
    show(text);
    _ui.wait_for_a();
}

void MessageBox::say_timed(const bn::string_view& text, int frames)
{
    show(text);
    _ui.wait_for_a_or(frames);
}

void MessageBox::say_pages(const bn::span<const bn::string_view>& pages)
{
    for(const bn::string_view& page : pages)
    {
        say(page);
    }
}

}
