#!/usr/bin/env python3
"""Placeholder asset generator for the Butano RPG framework.

Writes the indexed 16-color BMP files under ../graphics that Butano expects.
Everything here is "programmer art": replace any BMP with real pixel art
(same size, 16-color indexed, color 0 = transparent) and rebuild.

Usage:  python3 tools/gen_placeholder_assets.py
No third-party modules are required.
"""

import os
import struct

OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'graphics')


# ---------------------------------------------------------------------------
# BMP writer (4 bits per pixel, uncompressed: the format Butano/grit wants)
# ---------------------------------------------------------------------------

def write_bmp(name, pixels, palette):
    """pixels: list of rows (top to bottom) of palette indices (0..15)."""
    height = len(pixels)
    width = len(pixels[0])
    assert len(palette) == 16
    row_bytes = ((width * 4 + 31) // 32) * 4
    data = bytearray()

    for row in reversed(pixels):  # BMP rows are stored bottom-up.
        assert len(row) == width
        line = bytearray(row_bytes)

        for x, index in enumerate(row):
            if x % 2 == 0:
                line[x // 2] |= index << 4
            else:
                line[x // 2] |= index

        data += line

    offset = 14 + 40 + 16 * 4
    header = struct.pack('<2sIHHI', b'BM', offset + len(data), 0, 0, offset)
    info = struct.pack('<IiiHHIIiiII', 40, width, height, 1, 4, 0, len(data), 2835, 2835, 16, 16)
    colors = b''.join(struct.pack('<BBBB', b, g, r, 0) for (r, g, b) in palette)

    with open(os.path.join(OUT_DIR, name), 'wb') as file:
        file.write(header + info + colors + data)


def write_json(name, text):
    with open(os.path.join(OUT_DIR, name), 'w') as file:
        file.write(text)


# ---------------------------------------------------------------------------
# Tiny drawing helpers
# ---------------------------------------------------------------------------

def canvas(width, height, color=0):
    return [[color] * width for _ in range(height)]


def rect(c, x0, y0, x1, y1, color):
    """Filled rectangle, inclusive coordinates, clipped to the canvas."""
    for y in range(max(y0, 0), min(y1, len(c) - 1) + 1):
        for x in range(max(x0, 0), min(x1, len(c[0]) - 1) + 1):
            c[y][x] = color


def ellipse(c, cx, cy, rx, ry, color):
    for y in range(len(c)):
        for x in range(len(c[0])):
            if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1.0:
                c[y][x] = color


def triangle(c, p0, p1, p2, color):
    def edge(a, b, p):
        return (p[0] - a[0]) * (b[1] - a[1]) - (p[1] - a[1]) * (b[0] - a[0])

    for y in range(len(c)):
        for x in range(len(c[0])):
            e = [edge(p0, p1, (x, y)), edge(p1, p2, (x, y)), edge(p2, p0, (x, y))]
            if all(v >= 0 for v in e) or all(v <= 0 for v in e):
                c[y][x] = color


def outline(c, color=1):
    """Paints transparent pixels that touch an opaque one: cheap readable sprites."""
    height, width = len(c), len(c[0])
    marks = []

    for y in range(height):
        for x in range(width):
            if c[y][x] == 0:
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < width and 0 <= ny < height and c[ny][nx] not in (0, color):
                        marks.append((x, y))
                        break

    for x, y in marks:
        c[y][x] = color


def noise(x, y, seed):
    """Deterministic pseudo random value in [0, 255]."""
    v = (x * 374761393 + y * 668265263 + seed * 2246822519) & 0xFFFFFFFF
    v = ((v ^ (v >> 13)) * 1274126177) & 0xFFFFFFFF
    return (v ^ (v >> 16)) & 0xFF


def stack(frames):
    """Stacks equally sized frames vertically (Butano sprite sheet layout)."""
    rows = []
    for frame in frames:
        rows += frame
    return rows


# ---------------------------------------------------------------------------
# Field tileset: 16x16 map tiles. The order MUST match rpg::TileId (rpg_maps.h)
# ---------------------------------------------------------------------------

FIELD_PALETTE = [
    (255, 0, 255),    # 0 unused (transparent slot)
    (16, 16, 24),     # 1 black
    (24, 96, 40),     # 2 dark green
    (56, 152, 56),    # 3 green
    (120, 200, 88),   # 4 light green
    (120, 80, 40),    # 5 brown
    (216, 184, 120),  # 6 sand
    (24, 56, 136),    # 7 dark blue
    (48, 104, 208),   # 8 blue
    (136, 184, 248),  # 9 light blue
    (112, 112, 120),  # 10 gray
    (184, 184, 192),  # 11 light gray
    (248, 248, 248),  # 12 white
    (208, 56, 48),    # 13 red
    (136, 32, 32),    # 14 dark red
    (248, 216, 64),   # 15 yellow
]


def tile_void():
    return canvas(16, 16, 1)


def tile_grass(seed=1):
    c = canvas(16, 16, 3)
    for y in range(16):
        for x in range(16):
            n = noise(x, y, seed)
            if n < 20:
                c[y][x] = 2
            elif n > 236:
                c[y][x] = 4
    return c


def tile_path():
    c = canvas(16, 16, 6)
    for y in range(16):
        for x in range(16):
            if noise(x, y, 7) < 18:
                c[y][x] = 5
    return c


def tile_tree():
    c = tile_grass(2)
    rect(c, 7, 11, 8, 14, 5)
    ellipse(c, 7.5, 6.5, 6.5, 6, 2)
    ellipse(c, 6.5, 5.5, 4, 3.5, 3)
    ellipse(c, 5.5, 4.5, 1.5, 1.2, 4)
    return c


def tile_water():
    c = canvas(16, 16, 8)
    for y in (2, 10):
        rect(c, 1, y, 5, y, 9)
        rect(c, 9, y + 4, 13, y + 4, 9)
        rect(c, 6, y + 1, 7, y + 1, 7)
    return c


def tile_wall():
    c = canvas(16, 16, 11)
    for y in (3, 7, 11, 15):
        rect(c, 0, y, 15, y, 10)
    for row, y in enumerate((0, 4, 8, 12)):
        for x in ((3, 11) if row % 2 == 0 else (7, 15)):
            rect(c, x, y, x, y + 2, 10)
    return c


def tile_roof():
    c = canvas(16, 16, 13)
    for y in (3, 7, 11, 15):
        rect(c, 0, y, 15, y, 14)
    for row, y in enumerate((0, 4, 8, 12)):
        for x in ((0, 8) if row % 2 == 0 else (4, 12)):
            rect(c, x, y, x, y + 2, 14)
    return c


def tile_door():
    c = tile_wall()
    rect(c, 3, 2, 12, 15, 1)
    rect(c, 4, 3, 11, 15, 5)
    rect(c, 10, 9, 10, 10, 15)
    return c


def tile_sign():
    c = tile_grass(3)
    rect(c, 7, 9, 8, 14, 5)
    rect(c, 2, 2, 13, 9, 1)
    rect(c, 3, 3, 12, 8, 6)
    rect(c, 5, 5, 10, 5, 5)
    rect(c, 5, 7, 9, 7, 5)
    return c


def tile_rock():
    c = tile_grass(4)
    triangle(c, (8, 0), (0, 15), (15, 15), 10)
    triangle(c, (8, 0), (8, 15), (15, 15), 1)
    triangle(c, (8, 1), (9, 14), (14, 14), 10)
    triangle(c, (8, 1), (6, 6), (10, 6), 12)
    triangle(c, (7, 3), (2, 14), (7, 14), 11)
    return c


def tile_flowers():
    c = tile_grass(5)
    for (x, y, color) in ((3, 3, 13), (11, 4, 15), (6, 10, 15), (12, 12, 13)):
        rect(c, x - 1, y, x + 1, y, color)
        rect(c, x, y - 1, x, y + 1, color)
        c[y][x] = 12
    return c


def tile_tall_grass():
    c = canvas(16, 16, 2)
    for y in range(16):
        for x in range(16):
            if (x + (y // 4) * 2) % 4 == 0 and y % 4 != 3:
                c[y][x] = 4
            elif noise(x, y, 9) < 40:
                c[y][x] = 3
    return c


def tile_bridge():
    c = canvas(16, 16, 5)
    for x in (3, 7, 11, 15):
        rect(c, x, 0, x, 15, 1)
    rect(c, 0, 0, 15, 0, 6)
    rect(c, 0, 15, 15, 15, 6)
    return c


def tile_gate():
    c = tile_path()
    rect(c, 1, 1, 14, 15, 10)
    rect(c, 2, 2, 13, 15, 11)
    ellipse(c, 7.5, 8, 4.5, 5, 1)
    rect(c, 3, 8, 12, 15, 1)
    return c


def tile_spring():
    c = tile_grass(6)
    ellipse(c, 7.5, 8, 7.5, 6.5, 10)        # stone rim
    ellipse(c, 7.5, 8, 6.5, 5.5, 11)
    ellipse(c, 7.5, 8, 5, 4, 8)             # water
    rect(c, 4, 7, 6, 7, 9)
    rect(c, 9, 9, 11, 9, 9)
    c[6][8] = c[8][6] = c[9][10] = 12       # sparkles
    return c


def tile_cave_floor():
    c = canvas(16, 16, 10)
    for y in range(16):
        for x in range(16):
            n = noise(x, y, 11)
            if n < 22:
                c[y][x] = 11
            elif n > 238:
                c[y][x] = 1
    return c


def tile_cave_wall():
    c = canvas(16, 16, 1)
    for y in range(16):
        for x in range(16):
            n = noise(x, y, 12)
            if n < 55:
                c[y][x] = 7
            elif n > 236:
                c[y][x] = 10
    ellipse(c, 4, 4, 3.5, 2.5, 7)
    ellipse(c, 11.5, 10, 4, 3, 7)
    rect(c, 3, 3, 4, 3, 10)
    rect(c, 10, 9, 12, 9, 10)
    return c


def tile_stairs_down():
    c = tile_cave_floor()
    rect(c, 2, 2, 13, 13, 1)                # dark opening
    rect(c, 3, 4, 12, 4, 11)                # steps getting smaller as they go down
    rect(c, 4, 7, 11, 7, 11)
    rect(c, 5, 10, 10, 10, 11)
    return c


def tile_stairs_up():
    c = tile_cave_floor()
    rect(c, 2, 2, 13, 13, 10)
    rect(c, 3, 11, 12, 12, 11)              # light steps, bigger near the bottom
    rect(c, 4, 8, 11, 9, 12)
    rect(c, 5, 5, 10, 6, 11)
    rect(c, 6, 3, 9, 3, 12)
    return c


def tile_ash():
    c = canvas(16, 16, 10)
    for y in range(16):
        for x in range(16):
            n = noise(x, y, 21)
            if n < 70:
                c[y][x] = 1
            elif n > 225:
                c[y][x] = 11
            elif n > 215:
                c[y][x] = 14            # embers
    return c


def tile_rubble():
    c = tile_ash()
    rect(c, 1, 9, 14, 15, 1)                # collapsed wall
    rect(c, 2, 10, 6, 14, 5)
    rect(c, 8, 11, 13, 14, 10)
    rect(c, 3, 2, 4, 9, 1)                  # charred beams
    rect(c, 10, 4, 11, 10, 1)
    rect(c, 3, 2, 4, 3, 13)                 # still glowing
    c[4][10] = c[4][11] = 13
    return c


def tile_grave():
    c = tile_ash()
    rect(c, 7, 2, 8, 13, 5)                 # wooden cross
    rect(c, 4, 5, 11, 6, 5)
    rect(c, 3, 13, 12, 15, 1)               # fresh mound
    rect(c, 4, 13, 11, 14, 5)
    c[13][6] = 12                           # a flower
    c[13][7] = 15
    return c


def tile_altar():
    c = tile_cave_floor()
    rect(c, 1, 8, 14, 15, 11)               # stone table
    rect(c, 1, 8, 14, 9, 12)
    rect(c, 3, 11, 12, 15, 10)
    ellipse(c, 7.5, 5, 4, 4, 1)             # black mirror
    ellipse(c, 7.5, 5, 3, 3, 7)
    c[4][6] = 12                            # a star
    c[6][9] = 9
    return c


FIELD_TILES = [
    tile_void, tile_grass, tile_path, tile_tree, tile_water, tile_wall, tile_roof,
    tile_door, tile_sign, tile_rock, tile_flowers, tile_tall_grass, tile_bridge, tile_gate, tile_spring,
    tile_cave_floor, tile_cave_wall, tile_stairs_up, tile_stairs_down,
    tile_ash, tile_rubble, tile_grave, tile_altar,
]


def gen_field():
    # The sheet is 16 px wide, so every 16x16 map tile becomes four consecutive
    # 8x8 hardware tiles: [4n] top-left, [4n+1] top-right, [4n+2] bottom-left, [4n+3] bottom-right.
    write_bmp('field_tiles.bmp', stack([t() for t in FIELD_TILES]), FIELD_PALETTE)
    write_json('field_tiles.json', '{\n    "type": "regular_bg_tiles",\n    "bpp_mode": "bpp_4"\n}\n')
    write_bmp('field_palette.bmp', canvas(8, 8, 0), FIELD_PALETTE)
    write_json('field_palette.json',
               '{\n    "type": "bg_palette",\n    "bpp_mode": "bpp_4",\n    "colors_count": 16\n}\n')


# ---------------------------------------------------------------------------
# UI window tiles: 8x8 each, in a horizontal strip. Order matches rpg_ui.cpp
# ---------------------------------------------------------------------------

UI_PALETTE = [(255, 0, 255), (8, 8, 32), (248, 248, 248), (136, 136, 168)] + [(0, 0, 0)] * 12


def gen_ui():
    fill, line, shade = 1, 2, 3
    tiles = []

    def frame_tile(left, top, right, bottom):
        t = canvas(8, 8, fill)
        if top:
            rect(t, 0, 1, 7, 2, line)
            rect(t, 0, 3, 7, 3, shade)
        if bottom:
            rect(t, 0, 5, 7, 6, line)
            rect(t, 0, 4, 7, 4, shade)
        if left:
            rect(t, 1, 0, 2, 7, line)
            rect(t, 3, 0 if not top else 3, 3, 7 if not bottom else 4, shade)
        if right:
            rect(t, 5, 0, 6, 7, line)
            rect(t, 4, 0 if not top else 3, 4, 7 if not bottom else 4, shade)
        # Clean the outside of the corners so the frame looks rounded.
        if top:
            rect(t, 0, 0, 7, 0, fill)
            if left:
                rect(t, 0, 0, 0, 7, fill)
                rect(t, 1, 1, 1, 1, fill)
            if right:
                rect(t, 7, 0, 7, 7, fill)
                rect(t, 6, 1, 6, 1, fill)
        if bottom:
            rect(t, 0, 7, 7, 7, fill)
            if left:
                rect(t, 0, 0, 0, 7, fill)
                rect(t, 1, 6, 1, 6, fill)
            if right:
                rect(t, 7, 0, 7, 7, fill)
                rect(t, 6, 6, 6, 6, fill)
        if left and not top and not bottom:
            rect(t, 0, 0, 0, 7, fill)
        if right and not top and not bottom:
            rect(t, 7, 0, 7, 7, fill)
        return t

    tiles.append(canvas(8, 8, 0))                       # 0 transparent
    tiles.append(canvas(8, 8, fill))                    # 1 fill
    tiles.append(frame_tile(True, True, False, False))  # 2 top-left
    tiles.append(frame_tile(False, True, False, False)) # 3 top
    tiles.append(frame_tile(False, True, True, False))  # 4 top-right
    tiles.append(frame_tile(True, False, False, False)) # 5 left
    tiles.append(frame_tile(False, False, True, False)) # 6 right
    tiles.append(frame_tile(True, False, False, True))  # 7 bottom-left
    tiles.append(frame_tile(False, False, False, True)) # 8 bottom
    tiles.append(frame_tile(False, False, True, True))  # 9 bottom-right

    strip = [sum((t[y] for t in tiles), []) for y in range(8)]
    write_bmp('ui_tiles.bmp', strip, UI_PALETTE)
    write_json('ui_tiles.json', '{\n    "type": "regular_bg_tiles",\n    "bpp_mode": "bpp_4"\n}\n')
    write_bmp('ui_palette.bmp', canvas(8, 8, 0), UI_PALETTE)
    write_json('ui_palette.json',
               '{\n    "type": "bg_palette",\n    "bpp_mode": "bpp_4",\n    "colors_count": 16\n}\n')


# ---------------------------------------------------------------------------
# Sprites
# ---------------------------------------------------------------------------

SPRITE_PALETTE = [
    (255, 0, 255),    # 0 transparent
    (16, 16, 24),     # 1 black (outline)
    (248, 200, 152),  # 2 skin
    (48, 96, 216),    # 3 blue
    (24, 48, 128),    # 4 dark blue
    (120, 72, 32),    # 5 brown
    (248, 216, 64),   # 6 yellow
    (208, 56, 48),    # 7 red
    (56, 160, 64),    # 8 green
    (248, 248, 248),  # 9 white
    (160, 160, 168),  # 10 gray
    (88, 88, 96),     # 11 dark gray
    (144, 72, 184),   # 12 purple
    (240, 144, 48),   # 13 orange
    (136, 200, 248),  # 14 light blue
    (24, 96, 40),     # 15 dark green
]


def person(direction, step, hair, tunic, trim):
    """16x16 chibi character. direction: 'down', 'up', 'left', 'right'. step: 0 or 1."""
    c = canvas(16, 16)
    rect(c, 4, 1, 11, 7, 2)                      # head
    rect(c, 4, 1, 11, 3, hair)                   # hair / helmet
    rect(c, 4, 8, 11, 12, tunic)                 # body
    rect(c, 4, 10, 11, 10, trim)                 # belt

    if direction == 'down':
        rect(c, 4, 4, 4, 5, hair)
        rect(c, 11, 4, 11, 5, hair)
        c[5][6] = c[5][9] = 1
        rect(c, 3, 8, 3, 11, tunic)
        rect(c, 12, 8, 12, 11, tunic)
        c[11][3] = c[11][12] = 2
    elif direction == 'up':
        rect(c, 4, 1, 11, 6, hair)
        rect(c, 3, 8, 3, 11, tunic)
        rect(c, 12, 8, 12, 11, tunic)
        c[11][3] = c[11][12] = 2
    else:
        rect(c, 7, 1, 11, 6, hair)
        rect(c, 4, 1, 11, 2, hair)
        c[5][5] = 1
        rect(c, 7, 8, 8, 11, trim)
        c[11][7] = c[11][8] = 2

    # Legs: alternate which foot is lifted to fake a walk cycle.
    left_len, right_len = (3, 2) if step == 0 else (2, 3)
    rect(c, 5, 13, 6, 12 + left_len, 5)
    rect(c, 9, 13, 10, 12 + right_len, 5)

    if direction == 'right':
        c = [list(reversed(row)) for row in c]

    outline(c)
    return c


def gen_player():
    frames = [person(d, s, hair=4, tunic=3, trim=6)
              for d in ('down', 'up', 'left', 'right') for s in (0, 1)]
    write_bmp('player.bmp', stack(frames), SPRITE_PALETTE)
    write_json('player.json', '{\n    "type": "sprite",\n    "height": 16\n}\n')


def chest(opened):
    """16x16 treasure chest. Frame order in npc.bmp: closed, then open."""
    c = canvas(16, 16)
    rect(c, 2, 9, 13, 14, 5)                 # body
    if opened:
        rect(c, 2, 2, 13, 5, 13)             # lid, raised behind the chest
        rect(c, 2, 6, 13, 8, 1)              # dark inside
        rect(c, 4, 7, 11, 7, 6)              # gold glint
    else:
        rect(c, 2, 4, 13, 8, 13)             # lid
        rect(c, 2, 8, 13, 8, 1)              # seam
        rect(c, 7, 7, 8, 10, 6)              # lock
        c[9][7] = 1
    rect(c, 3, 9 if opened else 4, 3, 14, 6)    # metal bands
    rect(c, 12, 9 if opened else 4, 12, 14, 6)
    outline(c)
    return c


def gen_npcs():
    frames = [
        person('down', 0, hair=5, tunic=8, trim=6),     # 0 villager
        person('down', 0, hair=10, tunic=12, trim=9),   # 1 elder
        person('down', 0, hair=13, tunic=7, trim=9),    # 2 innkeeper
        chest(False),                                   # 3 closed chest
        chest(True),                                    # 4 open chest
        person('down', 0, hair=5, tunic=13, trim=9),    # 5 shopkeeper
        person('down', 0, hair=12, tunic=4, trim=14),   # 6 the partner (spearman)
        person('down', 0, hair=11, tunic=11, trim=7),   # 7 cultist
        person('down', 0, hair=7, tunic=7, trim=6),     # 8 Ash General
        person('down', 0, hair=6, tunic=14, trim=9),    # 9 the seer
    ]
    write_bmp('npc.bmp', stack(frames), SPRITE_PALETTE)
    write_json('npc.json', '{\n    "type": "sprite",\n    "height": 16\n}\n')


def gen_cursor():
    c = canvas(8, 8)
    for y in range(7):
        width = 4 - abs(3 - y)
        rect(c, 1, y, width, y, 9)
    outline(c)
    write_bmp('cursor.bmp', c, SPRITE_PALETTE)
    write_json('cursor.json', '{\n    "type": "sprite",\n    "height": 8\n}\n')


def eyes(c, cx, cy, gap):
    for x in (cx - gap, cx + gap):
        rect(c, x - 1, cy - 2, x + 1, cy + 1, 9)
        rect(c, x, cy - 1, x, cy + 1, 1)


def gen_enemies():
    # Slime
    c = canvas(32, 32)
    triangle(c, (16, 4), (8, 18), (24, 18), 3)
    ellipse(c, 16, 20, 12, 9, 3)
    ellipse(c, 11, 14, 2.5, 2, 14)
    rect(c, 5, 27, 26, 31, 0)
    ellipse(c, 16, 25, 11, 3.5, 3)
    eyes(c, 16, 19, 4)
    rect(c, 13, 24, 19, 24, 1)
    c[23][12] = c[23][20] = 1
    outline(c)
    write_bmp('enemy_slime.bmp', c, SPRITE_PALETTE)
    write_json('enemy_slime.json', '{\n    "type": "sprite",\n    "height": 32\n}\n')

    # Bat
    c = canvas(32, 32)
    triangle(c, (14, 14), (1, 6), (4, 22), 12)
    triangle(c, (17, 14), (30, 6), (27, 22), 12)
    triangle(c, (2, 8), (9, 14), (6, 20), 11)
    triangle(c, (29, 8), (22, 14), (25, 20), 11)
    ellipse(c, 15.5, 16, 6, 7, 12)
    triangle(c, (10, 11), (11, 5), (14, 10), 12)
    triangle(c, (21, 11), (20, 5), (17, 10), 12)
    eyes(c, 15, 15, 3)
    rect(c, 13, 19, 18, 19, 1)
    c[20][14] = c[20][17] = 9
    outline(c)
    write_bmp('enemy_bat.bmp', c, SPRITE_PALETTE)
    write_json('enemy_bat.json', '{\n    "type": "sprite",\n    "height": 32\n}\n')

    # Goblin
    c = canvas(32, 32)
    rect(c, 10, 17, 21, 26, 5)                       # body
    rect(c, 10, 22, 21, 23, 11)                      # belt
    rect(c, 11, 27, 14, 30, 8)                       # legs
    rect(c, 17, 27, 20, 30, 8)
    rect(c, 6, 18, 9, 23, 8)                         # arms
    rect(c, 22, 18, 25, 23, 8)
    rect(c, 25, 6, 27, 22, 5)                        # club
    rect(c, 24, 4, 28, 9, 5)
    ellipse(c, 15.5, 10, 7.5, 7, 8)                  # head
    triangle(c, (8, 9), (2, 5), (9, 13), 8)          # ears
    triangle(c, (23, 9), (29, 3), (22, 13), 8)
    eyes(c, 15, 9, 3)
    rect(c, 12, 14, 19, 14, 1)
    c[13][13] = c[13][18] = 9
    outline(c)
    write_bmp('enemy_goblin.bmp', c, SPRITE_PALETTE)
    write_json('enemy_goblin.json', '{\n    "type": "sprite",\n    "height": 32\n}\n')


def gen_skeleton():
    c = canvas(32, 32)
    ellipse(c, 16, 8, 7, 7, 9)              # skull
    rect(c, 11, 13, 20, 16, 9)              # jaw
    rect(c, 11, 7, 14, 10, 1)               # eye sockets
    rect(c, 18, 7, 21, 10, 1)
    c[12][16] = c[13][16] = 1               # nose
    rect(c, 12, 16, 20, 16, 10)             # teeth
    for x in (13, 15, 17, 19):
        c[16][x] = 1
    rect(c, 15, 17, 16, 27, 9)              # spine
    for y in (18, 21, 24):                  # ribs
        rect(c, 10, y, 21, y + 1, 9)
        rect(c, 12, y, 19, y, 10)
    rect(c, 6, 18, 8, 27, 9)                # arms
    rect(c, 23, 18, 25, 27, 9)
    rect(c, 11, 27, 13, 31, 9)              # legs
    rect(c, 18, 27, 20, 31, 9)
    rect(c, 4, 2, 5, 22, 10)                # a rusty sword
    rect(c, 2, 20, 7, 21, 5)
    outline(c)
    write_bmp('enemy_skeleton.bmp', c, SPRITE_PALETTE)
    write_json('enemy_skeleton.json', '{\n    "type": "sprite",\n    "height": 32\n}\n')


def gen_cult():
    # Cultist: a hooded figure in ash-gray with a torch. Used by the soldiers and by the scout.
    c = canvas(32, 32)
    triangle(c, (16, 2), (6, 30), (26, 30), 11)     # robe
    rect(c, 8, 26, 24, 30, 11)
    ellipse(c, 16, 9, 6, 6, 11)                     # hood
    ellipse(c, 16, 10, 4, 4, 1)                     # shadowed face
    rect(c, 13, 9, 14, 10, 7)                       # red eyes
    rect(c, 18, 9, 19, 10, 7)
    rect(c, 14, 17, 18, 18, 10)                     # crest: a crown...
    c[16][14] = c[16][16] = c[16][18] = 10
    rect(c, 13, 20, 19, 20, 7)                      # ...wreathed in ash
    rect(c, 26, 10, 27, 26, 5)                      # torch
    triangle(c, (26, 3), (23, 10), (30, 10), 13)
    triangle(c, (26, 6), (25, 10), (28, 10), 6)
    outline(c)
    write_bmp('enemy_cultist.bmp', c, SPRITE_PALETTE)
    write_json('enemy_cultist.json', '{\n    "type": "sprite",\n    "height": 32\n}\n')

    # Ash General: red plate armor, horned helmet, flaming greatsword.
    c = canvas(32, 32)
    rect(c, 9, 12, 22, 25, 7)                       # torso
    rect(c, 9, 18, 22, 19, 6)                       # belt
    rect(c, 5, 12, 8, 22, 7)                        # arms
    rect(c, 23, 12, 26, 22, 7)
    rect(c, 4, 10, 9, 13, 11)                       # pauldrons
    rect(c, 22, 10, 27, 13, 11)
    rect(c, 10, 26, 14, 31, 11)                     # legs
    rect(c, 17, 26, 21, 31, 11)
    ellipse(c, 15.5, 7, 5, 5, 11)                   # helmet
    triangle(c, (11, 5), (6, 0), (12, 8), 9)        # horns
    triangle(c, (20, 5), (25, 0), (19, 8), 9)
    rect(c, 12, 7, 19, 8, 1)                        # visor
    rect(c, 13, 7, 14, 7, 13)
    rect(c, 17, 7, 18, 7, 13)
    rect(c, 28, 2, 29, 24, 13)                      # flaming blade
    rect(c, 29, 4, 30, 20, 6)
    rect(c, 26, 22, 31, 23, 10)                     # hilt
    outline(c)
    write_bmp('enemy_drake.bmp', c, SPRITE_PALETTE)
    write_json('enemy_drake.json', '{\n    "type": "sprite",\n    "height": 32\n}\n')


def gen_guardian():
    # Star Guardian: a stone statue with a star on its chest.
    c = canvas(32, 32)
    rect(c, 8, 10, 23, 25, 10)                      # body
    rect(c, 3, 11, 7, 24, 10)                       # arms
    rect(c, 24, 11, 28, 24, 10)
    rect(c, 9, 26, 14, 31, 11)                      # legs
    rect(c, 17, 26, 22, 31, 11)
    rect(c, 11, 2, 20, 9, 10)                       # head
    rect(c, 12, 5, 19, 6, 1)                        # eye slit
    rect(c, 13, 5, 14, 6, 14)
    rect(c, 17, 5, 18, 6, 14)
    triangle(c, (15, 12), (10, 21), (21, 21), 6)    # star
    triangle(c, (15, 24), (10, 15), (21, 15), 6)
    rect(c, 8, 10, 23, 10, 9)                       # highlights
    rect(c, 11, 2, 20, 2, 9)
    outline(c)
    write_bmp('enemy_guardian.bmp', c, SPRITE_PALETTE)
    write_json('enemy_guardian.json', '{\n    "type": "sprite",\n    "height": 32\n}\n')


if __name__ == '__main__':
    os.makedirs(OUT_DIR, exist_ok=True)
    gen_field()
    gen_ui()
    gen_player()
    gen_npcs()
    gen_cursor()
    gen_enemies()
    gen_skeleton()
    gen_cult()
    gen_guardian()
    print('Placeholder assets written to', os.path.normpath(OUT_DIR))
