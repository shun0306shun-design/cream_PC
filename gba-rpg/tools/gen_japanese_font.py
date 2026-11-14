#!/usr/bin/env python3
"""Japanese font generator for the Butano RPG framework.

Builds the 16x16 variable width sprite font used when the game language is Japanese:

    graphics/rpg_japanese_16x16_font.bmp / .json    one 16x16 glyph per character, stacked vertically
    include/rpg_japanese_sprite_font.h              bn::sprite_font + UTF-8 character list + widths

The glyphs come from PixelMplus12 (tools/fonts, M+ FONT LICENSE): half width ASCII is 6 pixels wide and
kana/kanji are 12 pixels wide, so a text line of the message box holds 18 full width characters.

Which characters are included:
  - ASCII (always, a Butano sprite font needs '!'..'~').
  - All the hiragana, katakana and common punctuation (always): kana-only text never needs a new font.
  - Every other non-ASCII character found in the game sources (src/ and include/rpg_*.h), comments aside.

So after writing new Japanese text with kanji that weren't used before, run this again and rebuild:

    python3 tools/gen_japanese_font.py

Requires Pillow (pip install pillow). A character the font doesn't have is an error, not an empty box.
"""

import os
import re
import struct
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
FONT_PATH = os.path.join(ROOT, 'tools', 'fonts', 'PixelMplus12-Regular.ttf')
BMP_PATH = os.path.join(ROOT, 'graphics', 'rpg_japanese_16x16_font.bmp')
JSON_PATH = os.path.join(ROOT, 'graphics', 'rpg_japanese_16x16_font.json')
HEADER_NAME = 'rpg_japanese_sprite_font.h'
HEADER_PATH = os.path.join(ROOT, 'include', HEADER_NAME)

FONT_SIZE = 12          # PixelMplus12 is pixel exact at 12.
CELL = 16               # Sprite font cell (Butano supports 8x8, 8x16 and 16x16).
TOP = 2                 # Rows left empty over the glyphs: capitals then sit where the 8x16 font has them.
HALF_WIDTH = 6
FULL_WIDTH = 12

# Same 16 colors as the Butano common fonts, so both fonts look alike. 0 = transparent, 14 = white (text).
PALETTE = [
    (0, 255, 0), (49, 65, 95), (108, 56, 90), (176, 50, 67), (225, 57, 57), (96, 97, 97), (73, 101, 130),
    (240, 101, 70), (98, 131, 191), (97, 142, 188), (255, 146, 83), (134, 184, 165), (0, 0, 0), (188, 193, 195),
    (255, 255, 255), (15, 0, 0),
]
TEXT_COLOR = 14

# Always included, even if no text uses them yet.
BASE_CHARACTERS = (
    ''.join(chr(code) for code in range(0x3041, 0x3094))    # Hiragana.
    + ''.join(chr(code) for code in range(0x30A1, 0x30F7))  # Katakana.
    + '　、。，．・：；？！ー―…‥～「」『』（）〔〕【】＋－×÷＝＜＞％＆＊＠★☆●○◎◆◇■□▲△▼▽→←↑↓♪'
    + '０１２３４５６７８９'
)


def source_characters():
    """Non-ASCII characters of the game sources, comments stripped."""
    files = []

    for folder, _, names in os.walk(os.path.join(ROOT, 'src')):
        files += [os.path.join(folder, name) for name in names if name.endswith(('.cpp', '.h'))]

    include = os.path.join(ROOT, 'include')
    files += [os.path.join(include, name) for name in os.listdir(include)
              if name.startswith('rpg_') and name.endswith('.h') and name != HEADER_NAME]

    characters = set()

    for path in sorted(files):
        with open(path, encoding='utf-8') as file:
            text = file.read()

        text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
        text = re.sub(r'//[^\n]*', '', text)
        characters.update(character for character in text if ord(character) > 0x7F)

    return characters


def draw_glyph(font, character):
    """Returns (rows, width): CELL rows of CELL palette indices and the advance width in pixels."""
    image = Image.new('L', (CELL, CELL), 0)
    draw = ImageDraw.Draw(image)
    draw.fontmode = '1'     # No antialiasing.
    draw.text((0, TOP), character, font=font, fill=255)
    rows = [[TEXT_COLOR if image.getpixel((x, y)) else 0 for x in range(CELL)] for y in range(CELL)]
    return rows, round(font.getlength(character))


def render(font, character):
    """Like draw_glyph(), but it checks that the glyph fits a half or full width character."""
    rows, width = draw_glyph(font, character)

    if width not in (HALF_WIDTH, FULL_WIDTH):
        raise ValueError('Unexpected width %d for %r (U+%04X)' % (width, character, ord(character)))

    if any(row[x] for row in rows for x in range(width, CELL)):
        raise ValueError('%r (U+%04X) is wider than its advance width' % (character, ord(character)))

    return rows, width


def has_glyph(font, character, missing_glyph):
    if character.isspace():
        return True

    glyph = draw_glyph(font, character)
    return any(any(row) for row in glyph[0]) and glyph != missing_glyph


def write_bmp(path, pixels):
    """4 bits per pixel, uncompressed: what Butano/grit wants (same writer as gen_placeholder_assets.py)."""
    height = len(pixels)
    width = len(pixels[0])
    row_bytes = ((width * 4 + 31) // 32) * 4
    data = bytearray()

    for row in reversed(pixels):
        line = bytearray(row_bytes)

        for x, index in enumerate(row):
            line[x // 2] |= index << (4 if x % 2 == 0 else 0)

        data += line

    offset = 14 + 40 + 16 * 4
    header = struct.pack('<2sIHHI', b'BM', offset + len(data), 0, 0, offset)
    info = struct.pack('<IiiHHIIiiII', 40, width, height, 1, 4, 0, len(data), 2835, 2835, 16, 16)
    colors = b''.join(struct.pack('<BBBB', b, g, r, 0) for (r, g, b) in PALETTE)

    with open(path, 'wb') as file:
        file.write(header + info + colors + data)


def write_header(characters, widths):
    lines = [
        '/*',
        ' * Japanese sprite font (16x16 cells, variable width): ASCII + kana + the kanji used by the game text.',
        ' *',
        ' * GENERATED by tools/gen_japanese_font.py: do not edit. Run the script again after writing text with',
        ' * new kanji, or the missing characters stop the game with an error.',
        ' *',
        ' * Glyphs: PixelMplus12 (M+ FONT LICENSE, see tools/fonts/PixelMplus-LICENSE.txt).',
        ' */',
        '',
        '#ifndef RPG_JAPANESE_SPRITE_FONT_H',
        '#define RPG_JAPANESE_SPRITE_FONT_H',
        '',
        '#include "bn_sprite_font.h"',
        '#include "bn_utf8_characters_map.h"',
        '#include "bn_sprite_items_rpg_japanese_16x16_font.h"',
        '',
        'namespace rpg',
        '{',
        '',
        '// %d characters, in the order of the glyphs after the ASCII ones.' % len(characters),
        'constexpr bn::utf8_character japanese_sprite_font_utf8_characters[] = {',
    ]

    for start in range(0, len(characters), 16):
        row = ', '.join('"%s"' % character for character in characters[start:start + 16])
        lines.append('    %s,' % row)

    lines += [
        '};',
        '',
        '// Pixels per character: the space, the ASCII characters from \'!\' to \'~\' and the ones above.',
        'constexpr int8_t japanese_sprite_font_character_widths[] = {',
    ]

    for start in range(0, len(widths), 24):
        lines.append('    %s,' % ', '.join(str(width) for width in widths[start:start + 24]))

    lines += [
        '};',
        '',
        'constexpr bn::span<const bn::utf8_character> japanese_sprite_font_utf8_characters_span(',
        '        japanese_sprite_font_utf8_characters);',
        '',
        'constexpr auto japanese_sprite_font_utf8_characters_map =',
        '        bn::utf8_characters_map<japanese_sprite_font_utf8_characters_span>();',
        '',
        'constexpr bn::sprite_font japanese_sprite_font(',
        '        bn::sprite_items::rpg_japanese_16x16_font, japanese_sprite_font_utf8_characters_map.reference(),',
        '        japanese_sprite_font_character_widths);',
        '',
        '}',
        '',
        '#endif',
        '',
    ]

    with open(HEADER_PATH, 'w', encoding='utf-8', newline='\n') as file:
        file.write('\n'.join(lines))


def main():
    font = ImageFont.truetype(FONT_PATH, FONT_SIZE)
    missing_glyph = draw_glyph(font, chr(0xFFFE))   # What the font draws for a character it doesn't have.

    ascii_characters = [chr(code) for code in range(33, 127)]
    characters = sorted(set(BASE_CHARACTERS) | source_characters())
    missing = [character for character in characters if not has_glyph(font, character, missing_glyph)]

    if missing:
        sys.exit('The font has no glyph for: ' + ' '.join('%s (U+%04X)' % (c, ord(c)) for c in missing))

    pixels = []
    widths = [HALF_WIDTH]   # The space.

    for character in ascii_characters + characters:
        rows, width = render(font, character)
        pixels += rows
        widths.append(width)

    write_bmp(BMP_PATH, pixels)

    with open(JSON_PATH, 'w', newline='\n') as file:
        file.write('{\n    "type": "sprite",\n    "height": %d\n}\n' % CELL)

    write_header(characters, widths)

    kanji = sum(1 for character in characters if 0x4E00 <= ord(character) <= 0x9FFF)
    print('%d glyphs (%d ASCII + %d Japanese, of which %d kanji), %d KB of ROM' % (
        len(pixels) // CELL, len(ascii_characters), len(characters), kanji, len(pixels) * CELL // 2 // 1024))


if __name__ == '__main__':
    main()
