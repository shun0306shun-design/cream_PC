#!/usr/bin/env python3
"""Asset generator for the Butano RPG framework.

Writes the indexed 16-color BMP files under ../graphics that Butano expects.
The map tiles and the chests are hand-drawn pixel art, written as ASCII grids
in this file (see paint()), so the whole set keeps one palette and one style.
Characters and enemies are still "programmer art": replace any BMP with real
pixel art (same size, 16-color indexed, color 0 = transparent) and rebuild.

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
# Field tileset: 16x16 map tiles.
#
# The sheet has two parts:
#   1. One tile per rpg::TileId, in the same order (rpg_maps.h).
#   2. Graphics-only tiles that FieldScene picks by looking at the neighbours
#      (roof edges, house walls, torches, objects over another ground...).
#      Their order MUST match the "gfx_" constants of rpg_field_scene.cpp.
# ---------------------------------------------------------------------------

# Muted on purpose: the GBA screen and its classic RPGs use soft, slightly warm colors. The grays are warm
# too, so the ash of the burnt towns and the stone of the castles sit next to the greens without clashing.
FIELD_PALETTE = [
    (255, 0, 255),    # 0 unused (transparent slot)
    (36, 32, 46),     # 1 black (outlines, shadows)
    (46, 84, 62),     # 2 dark green
    (86, 134, 76),    # 3 green
    (142, 174, 100),  # 4 light green
    (118, 84, 58),    # 5 brown (wood)
    (206, 180, 132),  # 6 sand
    (46, 62, 110),    # 7 dark blue
    (76, 118, 176),   # 8 blue
    (150, 188, 216),  # 9 light blue
    (110, 104, 106),  # 10 gray
    (170, 162, 154),  # 11 light gray
    (234, 228, 210),  # 12 white
    (180, 88, 64),    # 13 red
    (110, 54, 46),    # 14 dark red (roof shadow, dark wood)
    (232, 194, 92),   # 15 yellow
]

# One character per pixel in the tile drawings below ('.' keeps what is under it).
PIXELS = {
    'k': 1, 'G': 2, 'g': 3, 'l': 4, 'b': 5, 's': 6, 'N': 7, 'u': 8,
    'c': 9, 'd': 10, 'e': 11, 'w': 12, 'r': 13, 'm': 14, 'y': 15,
}


def paint(c, rows, x0=0, y0=0):
    """Draws ASCII pixel art (see PIXELS) over a canvas."""
    for y, row in enumerate(rows):
        for x, char in enumerate(row):
            if char != '.':
                c[y0 + y][x0 + x] = PIXELS[char]
    return c


BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]
WARMER = {7: 8, 10: 11, 11: 6, 12: 15}


def glow(c, cx, cy, radius):
    """Torch light: brightens the pixels around (cx, cy), fading out with an ordered dither."""
    for y in range(16):
        for x in range(16):
            distance = ((x - cx) ** 2 + (y - cy) ** 2) ** 0.5
            if distance < radius and (BAYER[y % 4][x % 4] + 0.5) / 16 < 1 - distance / radius:
                c[y][x] = WARMER.get(c[y][x], c[y][x])
    return c


# --- Grounds -----------------------------------------------------------------

def tile_void():
    return canvas(16, 16, 1)


def tile_grass(seed=1):
    c = canvas(16, 16, 3)
    for y in range(16):
        for x in range(16):
            n = noise(x, y, seed)
            if n < 8:
                c[y][x] = 2
            elif n > 250:
                c[y][x] = 4
    # Tufts: two light blades over a dark root.
    for (x, y) in ((2, 3), (10, 1), (6, 8), (13, 10), (3, 13), (9, 14)):
        c[y][x] = c[y][x + 2] = 4
        c[y + 1][x + 1] = 2
    return c


def tile_path():
    c = canvas(16, 16, 6)
    for y in range(16):
        for x in range(16):
            n = noise(x, y, 7)
            if n < 7:
                c[y][x] = 5
            elif n > 249:
                c[y][x] = 12
    for (x, y) in ((3, 4), (11, 7), (6, 12)):                # pebbles: lit from the top left
        c[y][x] = 12
        c[y][x + 1] = c[y + 1][x] = 11
        c[y + 1][x + 1] = 5
    return c


def tile_ash():
    """Burnt ground: warm gray ash over scorched earth."""
    c = canvas(16, 16, 10)
    for y in range(16):
        for x in range(16):
            n = noise(x, y, 21)
            if n < 9:
                c[y][x] = 1
            elif n > 247:
                c[y][x] = 11
    for (x, y) in ((1, 2), (9, 5), (4, 10), (11, 13)):      # patches of earth showing through
        rect(c, x, y, x + 3, y, 5)
        rect(c, x + 1, y + 1, x + 4, y + 1, 5)
        c[y + 1][x] = 1
    for (x, y) in ((6, 1), (13, 9), (1, 7)):                 # light flakes of ash
        c[y][x] = c[y][x + 1] = 11
    c[3][13] = c[12][8] = 13                                 # embers
    c[4][13] = c[13][8] = 14
    return c


def tile_cave_floor():
    c = canvas(16, 16, 10)
    for y in range(16):
        for x in range(16):
            n = noise(x, y, 11)
            if n < 8:
                c[y][x] = 11
            elif n > 250:
                c[y][x] = 1
    for (x, y) in ((2, 3), (11, 10)):                        # flat stones: lit edge above, shadow under
        rect(c, x, y, x + 2, y, 11)
        rect(c, x, y + 1, x + 2, y + 1, 1)
    paint(c, ['k..', '.kk', '..k'], 10, 3)                   # cracks
    paint(c, ['.k', 'k.', 'k.'], 5, 11)
    return c


def tile_stone_floor():
    """Flagstones of forts, castles and cathedrals: every slab is lit from the top left."""
    c = canvas(16, 16, 11)
    slabs = ((0, 0, 8, 7), (9, 0, 15, 7), (0, 8, 4, 15), (5, 8, 12, 15), (13, 8, 15, 15))
    for (x0, y0, x1, y1) in slabs:
        rect(c, x0, y1, x1, y1, 10)             # joints
        rect(c, x1, y0, x1, y1, 10)
        rect(c, x0, y0, x1 - 1, y0, 12)         # lit edges
        rect(c, x0, y0, x0, y1 - 1, 12)
        c[y1 - 1][x1 - 1] = 10
    c[3][4] = c[4][5] = c[11][9] = 10                        # a little wear
    return c


GROUNDS = [tile_grass, tile_ash, tile_cave_floor, tile_stone_floor]    # Same order as Ground (rpg_field_scene.cpp).


# --- Nature ------------------------------------------------------------------

def tile_tree():
    return paint(tile_grass(2), [
        '.....kkkkkk.....',
        '...kkgglllgkk...',
        '..kGgllglllggk..',
        '.kGglgglgglgggk.',
        '.kgglggGgllglgk.',
        'kGgggGggglggggGk',
        'kGglgGGglgGglgGk',
        'kGgggGggggGgggGk',
        'kGGgGGgglgGGgGGk',
        'kGGGgGGgggGGGGGk',
        '.kGGGgGGgGGGGGk.',
        '.kkGGGGGGGGGGkk.',
        '..GkkkGbmGkkkG..',
        '..GGGkbbmmkGGG..',
        '...GkkbbbmkkG...',
        '.....kkkkkk.....',
    ])


def tile_rock():
    """Mountain of the overworld."""
    return paint(tile_grass(4), [
        '.......kk.......',
        '......kwwk......',
        '.....kwwwek.....',
        '.....kwweedk....',
        '....kweeeddk....',
        '....keeeedddk...',
        '...keeeeddddk...',
        '..kkeeeeddddkk..',
        '..keeedeedddddk.',
        '.keeeddeeddddkk.',
        '.keedddeedddddk.',
        'keeeddddedddddkk',
        'keedddddeeddddkk',
        'kedddddddeddddkk',
        'kkkkkkkkkkkkkkkk',
    ])


def tile_flowers():
    c = tile_grass(5)
    for (x, y, color) in ((3, 3, 13), (11, 4, 15), (6, 10, 15), (12, 12, 13), (2, 12, 12)):
        rect(c, x - 1, y, x + 1, y, color)
        rect(c, x, y - 1, x, y + 1, color)
        c[y][x] = 12 if color != 12 else 15
        c[y + 2][x] = 2
    return c


def tile_tall_grass():
    c = canvas(16, 16, 3)
    tuft = [
        '.l...l..',
        '.lG.lG..',
        'lGGlGG.l',
        'GGgGGglG',
        'GgGGgGGG',
        'gGgGGgGg',
        '........',
        '........',
    ]
    for (x0, y0) in ((0, 0), (8, 0), (-4, 8), (4, 8), (12, 8)):
        for y, row in enumerate(tuft):
            for x, char in enumerate(row):
                if char != '.' and 0 <= x0 + x < 16:
                    c[y0 + y][x0 + x] = PIXELS[char]
    return c


def tile_water(shore=False):
    c = canvas(16, 16, 8)
    # Ripples: a light crest over a dark trough, in a loose diagonal so the tile repeats without stripes.
    for (x, y, length) in ((1, 2, 5), (10, 5, 4), (4, 9, 5), (12, 12, 3), (0, 14, 3)):
        rect(c, x, y, x + length - 1, y, 9)
        c[y - 1][x + 1] = c[y - 1][x + length - 2] = 9
        rect(c, x + 1, y + 1, x + length, y + 1, 7)
    for (x, y) in ((7, 3), (14, 8), (2, 6), (9, 13)):       # glints
        c[y][x] = 12
        c[y][x + 1] = 9
    if shore:
        # The bank above casts a shadow, with a line of foam under it.
        rect(c, 0, 0, 15, 1, 7)
        for x in range(16):
            c[2][x] = 12 if x % 5 < 3 else 9
            c[3][x] = 9 if x % 5 in (0, 4) else 8
    return c


def tile_lava():
    c = canvas(16, 16, 13)
    for y in range(16):
        for x in range(16):
            n = noise(x, y, 31)
            if n < 50:
                c[y][x] = 14
    for (x, y) in ((1, 3), (9, 6), (3, 11), (11, 14)):
        rect(c, x, y, x + 3, y, 15)
        rect(c, x + 4, y + 1, x + 5, y + 1, 15)
    c[8][6] = c[1][13] = 12
    return c


def tile_bridge():
    c = canvas(16, 16, 5)
    for x in (3, 7, 11, 15):
        rect(c, x, 2, x, 13, 14)                # gaps between the planks
    for x in (1, 5, 9, 13):
        c[4][x] = c[11][x] = 6                  # nails
    for y in (0, 14):
        rect(c, 0, y, 15, y, 6)                 # handrails
        rect(c, 0, y + 1, 15, y + 1, 1)
    return c


# --- Buildings ---------------------------------------------------------------

def tile_wall():
    """Stone bricks: castle and town walls (houses use tile_house_wall)."""
    c = canvas(16, 16, 10)
    bricks = []
    for row, y in enumerate((0, 4, 8, 12)):
        edges = (-4, 4, 12, 20) if row % 2 == 0 else (0, 8, 16)
        bricks += [(edges[i], y, edges[i + 1] - 1) for i in range(len(edges) - 1)]
    for index, (x0, y, x1) in enumerate(bricks):
        rect(c, x0, y, x1 - 1, y, 11)           # lit top
        rect(c, x1, y, x1, y + 2, 1)            # mortar
        rect(c, x0, y + 3, x1, y + 3, 1)
        if index % 3 == 1:                      # some bricks are lighter, some are chipped
            rect(c, x0, y + 1, x1 - 1, y + 1, 11)
        elif index % 4 == 0:
            rect(c, x0 + 2, y + 2, x0 + 3, y + 2, 1)
    return c


TORCH = [
    '................',
    '.......y........',
    '......yyy.......',
    '......ywyy......',
    '.....yywwy......',
    '.....rywyr......',
    '......ryr.......',
    '.....kbbbk......',
    '......kbk.......',
    '......kbk.......',
    '.....kkbkk......',
    '......kkk.......',
]


def lit_floor(tile):
    """The floor under a torch."""
    return lambda: glow(tile(), 7.5, -5, 19)


def lit_wall(tile, torch_side):
    """The wall next to a torch (torch_side: -1 = the torch is at its left, 1 = at its right)."""
    return lambda: glow(tile(), 7.5 - (16 * -torch_side), 4, 19)


def tile_wall_torch():
    return paint(glow(tile_wall(), 7.5, 4, 11), TORCH)


def tile_wall_chain():
    return paint(tile_wall(), [
        '....kk.....kk...',
        '...kek....kek...',
        '...kdk....kdk...',
        '....ek.....ek...',
        '...kek....kek...',
        '...kdk....kdk...',
        '....ek.....ek...',
        '...kek....kek...',
        '...kdk....kdk...',
        '..keeek..keeek..',
        '..ke.ek..ke.ek..',
        '..keeek..keeek..',
        '...kkk....kkk...',
    ])


def tile_roof(edge=None):
    """Clay tiles in three rows, each lit at the top and shaded where the next row covers it."""
    c = canvas(16, 16, 13)
    rect(c, 0, 0, 15, 0, 1)                     # ridge
    rect(c, 0, 1, 15, 1, 6)
    rect(c, 0, 2, 15, 2, 14)
    for row in range(3):
        y = 3 + row * 4
        for x in range(16):
            column = (x + row * 4) % 8
            c[y][x] = 6 if column in (1, 2) else 13         # highlight
            c[y + 3][x] = 1 if column == 7 else 14          # shadow of the row
            if column == 7:
                c[y][x] = c[y + 1][x] = c[y + 2][x] = 14    # gap between two tiles
            elif column == 6:
                c[y + 2][x] = 14
    rect(c, 0, 15, 15, 15, 1)                   # eaves
    if edge == 'left':
        rect(c, 0, 0, 0, 15, 1)
        rect(c, 1, 1, 1, 14, 6)
        rect(c, 2, 1, 2, 14, 5)
    elif edge == 'right':
        rect(c, 15, 0, 15, 15, 1)
        rect(c, 14, 1, 14, 14, 5)
        rect(c, 13, 1, 13, 14, 6)
    return c


def tile_house_wall():
    """Half-timbered plaster over a stone base."""
    c = canvas(16, 16, 12)
    for y in range(16):
        for x in range(16):
            if noise(x, y, 51) < 10:
                c[y][x] = 11
    rect(c, 0, 0, 15, 0, 1)                     # shadow of the eaves
    rect(c, 0, 1, 15, 1, 10)
    rect(c, 0, 2, 15, 2, 11)
    rect(c, 0, 2, 0, 11, 5)                     # posts
    rect(c, 15, 2, 15, 11, 14)
    rect(c, 0, 11, 15, 11, 5)                   # sill beam
    rect(c, 0, 12, 15, 12, 14)
    rect(c, 0, 13, 15, 15, 10)                  # stone base
    rect(c, 0, 13, 15, 13, 11)
    for x in (5, 11):
        rect(c, x, 13, x, 14, 1)
    rect(c, 0, 15, 15, 15, 1)
    return c


def tile_house_window():
    return paint(tile_house_wall(), [
        '................',
        '................',
        '................',
        '...mmmmmmmmmm...',
        '..bmcc8bbc88mb..'.replace('8', 'u'),
        '..bmc88bb888mb..'.replace('8', 'u'),
        '..bm888bb88Nmb..'.replace('8', 'u'),
        '..bmbbbbbbbbmb..',
        '..bm888bb8NNmb..'.replace('8', 'u'),
        '..bm88NbbNNNmb..'.replace('8', 'u'),
        '.kssssssssssssk.',
        '.kgrgGgygGgrgGk.',
        '..kkkkkkkkkkkk..',
    ])


def tile_door():
    return paint(tile_house_wall(), [
        '................',
        '.....kkkkkk.....',
        '...kkeeeeeekk...',
        '..keekkkkkkeek..',
        '..kekbbmbbmkek..',
        '.kekbbbmbbmbkek.',
        '.kekbsbmbbmbkek.',
        '.kekbbbmbbmbkek.',
        '.kekmmmmmmmmkek.',
        '.kekbbbmbbmbkek.',
        '.kekbsbmbymbkek.',
        '.kekbbbmbymbkek.',
        '.kekbbbmbbmbkek.',
        '.kekbbbmbbmbkek.',
        'keeeeeeeeeeeeeek',
        'kddddddddddddddk',
    ])


def tile_inn_sign():
    """House wall with the hanging sign of an inn (a moon)."""
    return paint(tile_house_wall(), [
        '................',
        '..kkkkkkkkkkkk..',
        '....k......k....',
        '..kkkkkkkkkkkk..',
        '..kNNNNNNNNNNk..',
        '..kNNNyyyNNNNk..',
        '..kNNyyNNNNwNk..',
        '..kNNyyNNNNNNk..',
        '..kNNyyNNNwNNk..',
        '..kNNNyyyNNNNk..',
        '..kNNNNNNNNNNk..',
        '..kkkkkkkkkkkk..',
    ])


def tile_shop_sign():
    """House wall with the hanging sign of a shop (a potion)."""
    return paint(tile_house_wall(), [
        '................',
        '..kkkkkkkkkkkk..',
        '....k......k....',
        '..kkkkkkkkkkkk..',
        '..kssssbbssssk..',
        '..kssssccssssk..',
        '..ksssswcssssk..',
        '..ksssrrrrsssk..',
        '..kssrwrrrrssk..',
        '..kssrrrrrrssk..',
        '..ksssrrrrsssk..',
        '..kkkkkkkkkkkk..',
    ])


def tile_gate():
    """An archway: put a warp event on it."""
    return paint(tile_path(), [
        'kkkkkkkkkkkkkkkk',
        'keeeeeeeeeeeeeek',
        'kedddkkkkkkdddek',
        'keddkkkkkkkkddek',
        'kedkkkkkkkkkkdek',
        'kekkkkkkkkkkkkek',
        'kekkkkkkkkkkkkek',
        'kekkkkkkkkkkkkek',
        'kekkkkkkkkkkkkek',
        'kekkkkkkkkkkkkek',
        'kekkkkkkkkkkkkek',
        'kekkkkmmmmkkkkek',
        'kekkmmbbbbmmkkek',
        'kekmbbssssbbmkek',
        'kdkbssssssssbkdk',
        'kdkssssssssssbdk',
    ])


# --- Dungeons ----------------------------------------------------------------

def tile_cave_wall():
    """Boulders: a lit rim at the top left, dark gaps between them."""
    c = canvas(16, 16, 1)
    return paint(c, [
        '..dNNN....ddNN..',
        '.dNNNNN..dNNNNN.',
        '.NNNNNN..NNNNNN.',
        '.NNNNNk...NNNNk.',
        '..kkkk.....kkk..',
        'NN...dddNN...dNN',
        'NNN.dNNNNNN.dNNN',
        'NNN.NNNNNNN.NNNN',
        'NNk.NNNNNNk.NNNk',
        'kk...kkkkk...kkk',
        '..ddNNN...dNNN..',
        '.dNNNNNN.dNNNNN.',
        '.NNNNNNN.NNNNNN.',
        '.NNNNNNk..NNNNk.',
        '..kkkkk....kkk..',
        'N.....ddN.....dN',
    ])


def tile_cave_torch():
    return paint(glow(tile_cave_wall(), 7.5, 4, 11), TORCH)


def tile_stairs_down():
    return paint(tile_cave_floor(), [
        '................',
        '.kkkkkkkkkkkkkk.',
        '.keeeeeeeeeeeek.',
        '.kddddddddddddk.',
        '.kkeeeeeeeeeekk.',
        '.kkddddddddddkk.',
        '.kkkeeeeeeeekkk.',
        '.kkkddddddddkkk.',
        '.kkkkddddddkkkk.',
        '.kkkkkddddkkkkk.',
        '.kkkkkkddkkkkkk.',
        '.kkkkkkkkkkkkkk.',
        '.kkkkkkkkkkkkkk.',
        '.kkkkkkkkkkkkkk.',
        '................',
    ])


def tile_stairs_up():
    return paint(tile_cave_floor(), [
        '................',
        '.kkkkkkkkkkkkkk.',
        '.kkkkkwwwwkkkkk.',
        '.kkkkkeeeekkkkk.',
        '.kkkkwwwwwwkkkk.',
        '.kkkkeeeeeekkkk.',
        '.kkkwwwwwwwwkkk.',
        '.kkkeeeeeeeekkk.',
        '.kkwwwwwwwwwwkk.',
        '.kkeeeeeeeeeekk.',
        '.kwwwwwwwwwwwwk.',
        '.keeeeeeeeeeeek.',
        '.kddddddddddddk.',
        '.kkkkkkkkkkkkkk.',
        '................',
    ])


def tile_ladder():
    """Shown instead of stairs_up when the way up is against the cave wall."""
    return paint(tile_cave_floor(), [
        '..kkkkkkkkkkkk..',
        '.kkkkkkkkkkkkkk.',
        '.kkkbkkkkkkbkkk.',
        '.kkkbssssssbkkk.',
        '.kkkbkkkkkkbkkk.',
        '.kkkbkkkkkkbkkk.',
        '..kkbssssssbkk..',
        '....bmmmmmmb....',
        '....b......b....',
        '....bssssssb....',
        '....bmmmmmmb....',
        '....b......b....',
        '....bssssssb....',
        '....bmmmmmmb....',
        '....b......b....',
        '....k......k....',
    ])


def tile_rubble():
    """What is left of a burnt house."""
    return paint(tile_ash(), [
        '................',
        '...kk...........',
        '...kmk.....kk...',
        '...kmk....kmk...',
        '....kmk...kmk...',
        '....kmk..kmk....',
        '.....kmkkkmk....',
        '..kkkkmmmmkkkk..',
        '.kedkkkmmkkeddk.',
        'keeddkkkkkeedddk',
        'keddddkekkedddkk',
        'kkdddkeedkkddkek',
        'kedkkkedddkkkedk',
        'keddkeddddkeddkk',
        'kkkkkkkkkkkkkkkk',
        '................',
    ])


# --- Objects: drawn over the ground they stand on ------------------------------

def tile_sign(ground=tile_grass):
    return paint(ground(), [
        '................',
        '.kkkkkkkkkkkkkk.',
        '.kssssssssssssk.',
        '.ksbbbbsbbbbssk.',
        '.kssssssssssssk.',
        '.ksbbbsbbbbbssk.',
        '.kssssssssssssk.',
        '.ksbbbbbsbbsssk.',
        '.kbbbbbbbbbbbbk.',
        '.kkkkkkkkkkkkkk.',
        '......kbmk......',
        '......kbmk......',
        '......kbmk......',
        '......kbmk......',
        '.....kkkkkk.....',
    ])


def tile_spring(ground=tile_grass):
    """A fountain: put a spring event on it."""
    return paint(ground(), [
        '.......ww.......',
        '......wccw......',
        '.....c.ww.c.....',
        '....c..ee..c....',
        '...kkkkedkkkk...',
        '.kkeeeeedeeeekk.',
        'kewcccceecccccek',
        'kecwuucwwcuucudk',
        'kecuuccuuccuuudk',
        'kecuwuuuuuucwudk',
        'kedcuuccuccuuddk',
        'kkeddcuuuucddekk',
        'kdkeeedddddeekdk',
        '.kddkeeeeeekddk.',
        '..kkddddddddkk..',
        '....kkkkkkkk....',
    ])


def tile_grave(ground=tile_ash):
    return paint(ground(), [
        '................',
        '......kkkk......',
        '......kbmk......',
        '......kbmk......',
        '..kkkkkbmkkkkk..',
        '..kbbbbbbbbbmk..',
        '..kmmmmbmmmmmk..',
        '..kkkkkbmkkkkk..',
        '......kbmk......',
        '......kbmk......',
        '......kbmk......',
        '...kkkkbmkkkk...',
        '..kbbsbbbbbbmk..',
        '.kbbbbbbwybbbmk.',
        '.kmmmmmmmGmmmmk.',
        '..kkkkkkkkkkkk..',
    ])


def tile_altar(ground=tile_cave_floor):
    return paint(ground(), [
        '.....kkkkkk.....',
        '....kNNNNNNk....',
        '...kNNwNNNNNk...',
        '...kNwwwNNcNk...',
        '...kNNwNNNNNk...',
        '...kNNNNcNNNk...',
        '....kNNNNNNk....',
        '.kkkkkkkkkkkkkk.',
        '.kwwwwwwwwwwwwk.',
        '.keeeeeeeeeeeek.',
        '.kkkkkkkkkkkkkk.',
        '..kdeeddddeedk..',
        '..kdeeddddeedk..',
        '..kdeeddddeedk..',
        '.kkeeeeeeeeeekk.',
        '.kkkkkkkkkkkkkk.',
    ])


def tile_barrel(ground=tile_grass):
    return paint(ground(), [
        '................',
        '....kkkkkkkk....',
        '...kbmmmmmmbk...',
        '..kbbbbbbbbbbk..',
        '..kbsbbmbbmbmk..',
        '..keeeeeeeeeek..',
        '.kbbsbbmbbmbbmk.',
        '.kbbsbbmbbmbbmk.',
        '.kbbsbbmbbmbbmk.',
        '.kbbsbbmbbmbbmk.',
        '..keeeeeeeeeek..',
        '..kbsbbmbbmbmk..',
        '..kbbbbmbbmbmk..',
        '...kmmmmmmmmk...',
        '....kkkkkkkk....',
        '................',
    ])


def tile_magic_circle(ground=tile_stone_floor):
    """A warp: walkable, put a warp event on it."""
    return paint(ground(), [
        '................',
        '.....cccccc.....',
        '...ccNNNNNNcc...',
        '..cNNNNwwNNNNc..',
        '..cNNNwccwNNNc..',
        '.cNNwwwccwwwNNc.',
        '.cNNNwccccwNNNc.',
        '.cNNNNwccwNNNNc.',
        '.cNNNwccccwNNNc.',
        '.cNNwwcNNcwwNNc.',
        '.cNNwcNNNNcwNNc.',
        '..cNNNNNNNNNNc..',
        '..cNNNNNNNNNNc..',
        '...ccNNNNNNcc...',
        '.....cccccc.....',
        '................',
    ])


# Objects that FieldScene draws over the ground around them. Same order as GroundObject (rpg_field_scene.cpp).
GROUND_OBJECTS = [tile_sign, tile_spring, tile_grave, tile_altar, tile_barrel, tile_magic_circle]


# --- Village decorations (always on grass) -------------------------------------

def tile_fence():
    return paint(tile_grass(8), [
        '................',
        '................',
        '.kk....kk....kk.',
        'kbbk..kbbk..kbbk',
        'kbmk..kbmk..kbmk',
        'kbbkkkkbbkkkkbbk',
        'sbmsssssbmssssbm',
        'kbmkkkkbmkkkkbmk',
        'kbmk..kbmk..kbmk',
        'kbbkkkkbbkkkkbbk',
        'sbmsssssbmssssbm',
        'kbmkkkkbmkkkkbmk',
        'kbmk..kbmk..kbmk',
        'kbmk..kbmk..kbmk',
        '.kk....kk....kk.',
        '................',
    ])


def tile_lamp():
    return paint(tile_grass(9), [
        '......kkkk......',
        '.....kddddk.....',
        '....kkkkkkkk....',
        '....kywwyyyk....',
        '....kywwyyyk....',
        '....kyyyyyrk....',
        '....kkyyyrkk....',
        '.....kkkkkk.....',
        '......kedk......',
        '......kedk......',
        '......kedk......',
        '......kedk......',
        '......kedk......',
        '.....kkedkk.....',
        '....kdeeeddk....',
        '....kkkkkkkk....',
    ])


def tile_bench():
    return paint(tile_grass(10), [
        '................',
        '................',
        '................',
        '.kkkkkkkkkkkkkk.',
        '.kbbbbbbbbbbbbk.',
        '.kmmmmmmmmmmmmk.',
        '.kbbbbbbbbbbbbk.',
        '.kmmmmmmmmmmmmk.',
        '.kkkkkkkkkkkkkk.',
        'kssssssssssssssk',
        'kbbbbbbbbbbbbbbk',
        'kkkkkkkkkkkkkkkk',
        '.kbk........kbk.',
        '.kmk........kmk.',
        '.kkk........kkk.',
        '................',
    ])


def tile_well():
    return paint(tile_grass(11), [
        '..kkkkkkkkkkkk..',
        '.krrrrrrrrrrrrk.',
        'krrmrrrmrrrmrrrk',
        'kkkkkkkkkkkkkkkk',
        '..kb...kk...bk..',
        '..kb...kd...bk..',
        '..kb..kbbk..bk..',
        '..kb..kbbk..bk..',
        '.kkkkkkkkkkkkkk.',
        'keedeeeeeedeeedk',
        'kdddkkkkkkkkdddk',
        'keekNNuuuuNNkedk',
        'keedkkkkkkkkeedk',
        'kdeeeedeeeeeeddk',
        'kddeddddddedddkk',
        '.kkkkkkkkkkkkkk.',
    ])


# Part 1: one tile per rpg::TileId, in the same order.
FIELD_TILES = [
    tile_void, tile_grass, tile_path, tile_tree, tile_water, tile_wall, tile_roof,
    tile_door, tile_sign, tile_rock, tile_flowers, tile_tall_grass, tile_bridge, tile_gate, tile_spring,
    tile_cave_floor, tile_cave_wall, tile_stairs_up, tile_stairs_down,
    tile_ash, tile_rubble, tile_grave, tile_altar, tile_lava,
    tile_fence, tile_lamp, tile_bench, tile_well, tile_barrel, tile_magic_circle, tile_inn_sign, tile_shop_sign,
]

# Part 2: graphics-only tiles. Same order as the "gfx_" constants of rpg_field_scene.cpp.
FIELD_EXTRA_TILES = [
    lambda: tile_roof('left'), lambda: tile_roof('right'), tile_house_wall, tile_house_window,
    tile_stone_floor, tile_wall_torch, tile_wall_chain, tile_cave_torch, tile_ladder,
    lambda: tile_water(shore=True),
    lit_floor(tile_stone_floor), lit_floor(tile_cave_floor),
    lit_wall(tile_wall, -1), lit_wall(tile_wall, 1), lit_wall(tile_cave_wall, -1), lit_wall(tile_cave_wall, 1),
]


def field_sheet():
    """All the field tiles, in sheet order."""
    tiles = [t() for t in FIELD_TILES] + [t() for t in FIELD_EXTRA_TILES]
    # Every ground object over every ground: index = base + (object * len(GROUNDS)) + ground.
    tiles += [obj(ground) for obj in GROUND_OBJECTS for ground in GROUNDS]
    return tiles


def gen_field():
    # The sheet is 16 px wide, so every 16x16 map tile becomes four consecutive
    # 8x8 hardware tiles: [4n] top-left, [4n+1] top-right, [4n+2] bottom-left, [4n+3] bottom-right.
    write_bmp('field_tiles.bmp', stack(field_sheet()), FIELD_PALETTE)
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
    pixels = {'k': 1, 'b': 5, 'y': 6, 'r': 7, 'w': 9, 'd': 11, 'o': 13}
    closed = [
        '................',
        '................',
        '...bbbbbbbbbb...',
        '..boooooooooob..',
        '.byoooooooooyob.',
        '.byobbbbbbbboyb.',
        '.byooooooooooyb.',
        '.bbbbbbyybbbbbb.',
        '.kkkkkkywkkkkkk.',
        '.byoooyykyoooyb.',
        '.byooooyyooooyb.',
        '.byooooooooooyb.',
        '.byobbbbbbbboyb.',
        '.byooooooooooyb.',
        '.bbbbbbbbbbbbbb.',
        '................',
    ]
    open_ = [
        '..bbbbbbbbbbbb..',
        '.byoooooooooyob.',
        '.byobbbbbbbboyb.',
        '.byooooooooooyb.',
        '.bbbbbbbbbbbbbb.',
        '.bddddddddddddb.',
        '.bkkkkkkkkkkkkb.',
        '.bkkywykkyykwkb.',
        '.bkyyyyywyyyykb.',
        '.byoooooooooyob.',
        '.byooooooooooyb.',
        '.byooooooooooyb.',
        '.byobbbbbbbboyb.',
        '.byooooooooooyb.',
        '.bbbbbbbbbbbbbb.',
        '................',
    ]
    c = canvas(16, 16)
    for y, row in enumerate(open_ if opened else closed):
        for x, char in enumerate(row):
            if char != '.':
                c[y][x] = pixels[char]
    outline(c)
    return c


def wyrm_head():
    """16x16 map sprite of the canyon boss: a serpent head rising from the lava."""
    c = canvas(16, 16)
    ellipse(c, 7.5, 7, 6, 6, 7)
    triangle(c, (3, 3), (1, 0), (6, 2), 13)     # horns
    triangle(c, (12, 3), (14, 0), (9, 2), 13)
    rect(c, 4, 6, 5, 7, 6)                      # eyes
    rect(c, 10, 6, 11, 7, 6)
    rect(c, 5, 10, 10, 11, 1)                   # mouth
    rect(c, 3, 13, 12, 15, 13)                  # lava around the neck
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
        wyrm_head(),                                    # 10 ember wyrm
        person('down', 0, hair=9, tunic=12, trim=6),    # 11 the stranger (after the ending)
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


def gen_wyrm():
    # Ember Wyrm: a serpent of cooling lava.
    c = canvas(32, 32)
    ellipse(c, 16, 24, 13, 6, 7)                    # coils
    ellipse(c, 16, 24, 8, 3, 13)
    rect(c, 13, 10, 19, 24, 7)                      # neck
    rect(c, 15, 12, 17, 23, 13)
    ellipse(c, 16, 8, 8, 6, 7)                      # head
    triangle(c, (10, 5), (5, 0), (13, 4), 13)       # horns
    triangle(c, (21, 5), (26, 0), (18, 4), 13)
    eyes(c, 16, 8, 4)
    rect(c, 12, 11, 20, 12, 1)                      # mouth
    rect(c, 13, 12, 14, 13, 9)                      # fangs
    rect(c, 18, 12, 19, 13, 9)
    triangle(c, (16, 13), (14, 18), (18, 18), 6)    # fire breath
    outline(c)
    write_bmp('enemy_wyrm.bmp', c, SPRITE_PALETTE)
    write_json('enemy_wyrm.json', '{\n    "type": "sprite",\n    "height": 32\n}\n')


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
    gen_wyrm()
    print('Placeholder assets written to', os.path.normpath(OUT_DIR))
