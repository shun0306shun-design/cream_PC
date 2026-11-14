/*
 * Game language and bilingual text.
 *
 * A text that exists in both languages is written once, where it is used, with RPG_T:
 *
 *     box.say(RPG_T("The chest is empty.", "宝箱は空っぽだ。"));
 *
 * RPG_T joins the two string literals with a separator, so the result is still one constexpr string stored in ROM
 * and every table of bn::string_view (pages, menu options, map names...) keeps its type. The windows of rpg_ui.h
 * show only the half of the current language (localized()); a plain text without separator is shown as it is,
 * so what hasn't been translated yet keeps working in both languages.
 *
 * Japanese text is UTF-8 and is drawn with the font of rpg_japanese_sprite_font.h, which only has the kanji that
 * the sources use: run tools/gen_japanese_font.py after writing new text.
 * Limits of a Japanese message box page: 18 full width characters per line and 3 lines (they are then
 * spread over the box, see MessageBox::japanese_line_pitch; a 4 lines page is packed like English text).
 *
 * Text built at run time (numbers, item names) can't use RPG_T on the whole sentence: choose the pieces
 * with tr() instead.
 */

#ifndef RPG_TEXT_H
#define RPG_TEXT_H

#include "bn_string_view.h"

/// English and Japanese versions of a text as one string literal: "english\fjapanese".
#define RPG_T(english, japanese) english "\f" japanese

namespace rpg
{

enum class Language : uint8_t
{
    japanese,
    english
};

constexpr Language default_language = Language::japanese;
constexpr char language_separator = '\f';

[[nodiscard]] Language language();

/// Texts shown from now on use the given language (what is already on screen doesn't change).
void set_language(Language language);

/// Returns the part of a RPG_T text for the current language (or the whole text if it has only one part).
[[nodiscard]] bn::string_view localized(const bn::string_view& text);

/// Chooses between two run time texts.
[[nodiscard]] inline bn::string_view tr(const bn::string_view& english, const bn::string_view& japanese)
{
    return language() == Language::english ? english : japanese;
}

}

#endif
