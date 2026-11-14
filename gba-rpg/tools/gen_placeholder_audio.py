#!/usr/bin/env python3
"""Placeholder audio generator for the Butano RPG framework.

Writes short chiptune style placeholders in audio/, only with the standard library:

    se_*.wav    sound effects (8 bit unsigned mono PCM)
    bgm_*.mod   music loops (4 channel ProTracker modules, two patterns each)

They exist so that the game "makes sound" and every hook in the code can be heard. To replace one with real
audio, overwrite the file keeping its name (music can also be .xm, .it or .s3m: delete the .mod then) and
rebuild. Which file is used for what is decided in src/rpg_audio.cpp.

Running this again overwrites all the placeholders:

    python3 tools/gen_placeholder_audio.py
"""

import math
import os
import random
import struct
import wave

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
AUDIO = os.path.join(ROOT, 'audio')

# ----------------------------------------------------------------------------------------------------------------------
# Sound effects
# ----------------------------------------------------------------------------------------------------------------------

SE_RATE = 16000


def square(phase, duty=0.5):
    return 1.0 if (phase % 1.0) < duty else -1.0


def render(segments, noise=0.0, duty=0.5, decay=0.0, seed=1):
    """Renders (start_hz, end_hz, seconds) segments: a square wave that slides, mixed with noise, fading out."""
    rng = random.Random(seed)
    total = sum(int(seconds * SE_RATE) for _, _, seconds in segments)
    samples = []
    phase = 0.0

    for start_hz, end_hz, seconds in segments:
        count = int(seconds * SE_RATE)

        for index in range(count):
            hz = start_hz + (end_hz - start_hz) * index / count
            phase += hz / SE_RATE
            tone = square(phase, duty) if hz > 0 else 0.0
            value = tone * (1.0 - noise) + rng.uniform(-1.0, 1.0) * noise
            position = len(samples) / total
            envelope = (1.0 - position) ** decay if decay else 1.0

            # Short fade at both ends of the whole effect: no clicks.
            edge = min(len(samples), total - 1 - len(samples)) / (0.004 * SE_RATE)
            samples.append(value * envelope * min(1.0, edge))

    return samples


def write_wav(name, samples, volume=0.6):
    data = bytes(max(0, min(255, int(128 + sample * volume * 127))) for sample in samples)

    with wave.open(os.path.join(AUDIO, name + '.wav'), 'wb') as wav:
        wav.setnchannels(1)
        wav.setsampwidth(1)
        wav.setframerate(SE_RATE)
        wav.writeframes(data)


def note_hz(semitones_from_a4):
    return 440.0 * 2 ** (semitones_from_a4 / 12)


def gen_sound_effects():
    # Confirm: two quick rising blips.
    write_wav('se_confirm', render([(880, 880, 0.04), (1320, 1320, 0.06)], duty=0.25, decay=0.5))

    # Cancel: the same, falling.
    write_wav('se_cancel', render([(660, 660, 0.04), (440, 440, 0.07)], duty=0.25, decay=0.5))

    # Attack: a noisy swoosh that falls.
    write_wav('se_attack', render([(900, 200, 0.12)], noise=0.7, decay=1.5, seed=2))

    # Damage: a low thud.
    write_wav('se_damage', render([(160, 60, 0.18)], noise=0.45, decay=1.2, seed=3), volume=0.75)

    # Level up: a major arpeggio that ends on a held note.
    arpeggio = [(note_hz(n), note_hz(n), 0.07) for n in (3, 7, 10, 15)] + [(note_hz(15), note_hz(15), 0.22)]
    write_wav('se_level_up', render(arpeggio, duty=0.5, decay=0.6))

    # Chest: a creak, then a small sparkle.
    chest = [(220, 330, 0.08), (0, 0, 0.03), (note_hz(10), note_hz(10), 0.06), (note_hz(17), note_hz(17), 0.14)]
    write_wav('se_chest', render(chest, duty=0.25, decay=0.5))


# ----------------------------------------------------------------------------------------------------------------------
# Music: ProTracker modules
# ----------------------------------------------------------------------------------------------------------------------

PERIODS = [
    856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,     # Octave 1
    428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,     # Octave 2
    214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,     # Octave 3
]
NOTE_NAMES = {'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5, 'F#': 6, 'G': 7, 'G#': 8, 'A': 9, 'A#': 10, 'B': 11}

ROWS = 64
ROWS_PER_TOKEN = 2      # A token of the note strings is an eighth note.

# Instruments (1 based, like in the module). A 32 samples cycle played as "C-2" sounds as C4.
SQUARE, TRIANGLE, BASS, NOISE = 1, 2, 3, 4


def cycle_sample(function, cycle, cycles=4):
    return bytes(int(function(index / cycle) * 100) & 0xFF for index in range(cycle * cycles))


def make_samples():
    rng = random.Random(7)
    triangle = lambda phase: 4 * abs((phase % 1.0) - 0.5) - 1
    noise = bytes(int(rng.uniform(-1, 1) * 110 * (1 - index / 900) ** 2) & 0xFF for index in range(900))

    return [
        # name, data, volume, looped
        ('square', cycle_sample(lambda phase: square(phase, 0.25), 32), 40, True),
        ('triangle', cycle_sample(triangle, 32), 64, True),
        ('bass', cycle_sample(triangle, 64), 64, True),            # One octave lower.
        ('noise', noise, 48, False),
    ]


def period_of(name, octave_shift):
    """'F#4' -> module period. octave_shift maps written octaves to the three octaves a module has."""
    octave = int(name[-1]) - octave_shift
    index = (octave - 1) * 12 + NOTE_NAMES[name[:-1]]

    if not 0 <= index < len(PERIODS):
        raise ValueError('Note out of range: ' + name)

    return PERIODS[index]


def cell(period=0, sample=0, effect=0, param=0):
    return bytes([(sample & 0xF0) | (period >> 8), period & 0xFF, ((sample & 0x0F) << 4) | effect, param])


def melody_cells(text, sample, octave_shift, volume):
    """A row list from a note string: 'C5' plays, '.' holds the last note, '-' silences the channel."""
    tokens = text.replace('|', ' ').split()

    if len(tokens) * ROWS_PER_TOKEN != ROWS:
        raise ValueError('A pattern needs %d tokens, got %d: %s' % (ROWS // ROWS_PER_TOKEN, len(tokens), text))

    rows = []

    for token in tokens:
        if token == '.':
            rows.append(cell())
        elif token == '-':
            rows.append(cell(effect=0xC, param=0))
        else:
            rows.append(cell(period_of(token, octave_shift), sample, 0xC, volume))

        rows.extend([cell()] * (ROWS_PER_TOKEN - 1))

    return rows


def drum_cells(bar):
    """`bar` has one character per eighth note and is repeated: k = kick, s = snare, h = hi-hat, - = nothing."""
    drums = {'k': (PERIODS[0], 56), 's': (PERIODS[14], 44), 'h': (PERIODS[35], 20)}
    rows = []

    for token in range(ROWS // ROWS_PER_TOKEN):
        drum = drums.get(bar[token % len(bar)])
        rows.append(cell(drum[0], NOISE, 0xC, drum[1]) if drum else cell())
        rows.extend([cell()] * (ROWS_PER_TOKEN - 1))

    return rows


def write_mod(name, title, tempo, patterns):
    """patterns: a list of (lead, bass, drums bar) per pattern, plus the lead instrument as first element."""
    lead_sample = patterns[0]
    samples = make_samples()
    data = title.encode('ascii')[:20].ljust(20, b'\0')

    for index in range(31):
        if index < len(samples):
            sample_name, sample_data, volume, looped = samples[index]
            words = len(sample_data) // 2
            data += sample_name.encode('ascii').ljust(22, b'\0')
            data += struct.pack('>HBBHH', words, 0, volume, 0, words if looped else 1)
        else:
            data += bytes(22) + struct.pack('>HBBHH', 0, 0, 0, 0, 1)

    song = patterns[1:]
    data += bytes([len(song), 127]) + bytes(range(len(song))).ljust(128, b'\0') + b'M.K.'

    for lead, bass, drums in song:
        lead_rows = melody_cells(lead, lead_sample, 2, 48)
        # The second voice repeats the lead one eighth later and softer: a cheap echo that fills the sound.
        echo_rows = [cell()] * ROWS_PER_TOKEN + melody_cells(lead, lead_sample, 2, 16)[:-ROWS_PER_TOKEN]
        bass_rows = melody_cells(bass, BASS, 1, 56)
        drum_rows = drum_cells(drums)

        # Speed and tempo on the first row, in the two channels that never need an effect there.
        echo_rows[0] = cell(effect=0xF, param=6)
        drum_first = drum_rows[0]

        if drum_first[2] & 0x0F:
            # The first row has a drum with a volume command: move the tempo command to the next row.
            drum_rows[1] = cell(effect=0xF, param=tempo)
        else:
            drum_rows[0] = cell(effect=0xF, param=tempo)

        for row in range(ROWS):
            data += lead_rows[row] + echo_rows[row] + bass_rows[row] + drum_rows[row]

    for _, sample_data, _, _ in samples:
        data += sample_data

    with open(os.path.join(AUDIO, name + '.mod'), 'wb') as mod:
        mod.write(data)


def gen_music():
    # Town: bright, C major.
    write_mod('bgm_town', 'placeholder town', 112, [
        SQUARE,
        ('E5 . G5 . E5 . C5 D5 | E5 . F5 . A5 . F5 . | D5 . F5 . D5 . B4 C5 | D5 . E5 . G5 . . .',
         'C3 . G3 . C3 . G3 . | F2 . C3 . F2 . C3 . | G2 . D3 . G2 . D3 . | C3 . G3 . C3 . G2 .',
         'k-h-s-h-'),
        ('A5 . G5 . E5 . G5 . | F5 . E5 . C5 . E5 . | D5 . E5 F5 G5 . B4 . | C5 . . . C5 . - .',
         'F2 . C3 . C3 . G3 . | F2 . C3 . A2 . E3 . | G2 . D3 . G2 . D3 . | C3 . G2 . C3 . - .',
         'k-h-s-h-'),
    ])

    # Dungeon: slow and empty, A minor.
    write_mod('bgm_dungeon', 'placeholder dungeon', 80, [
        TRIANGLE,
        ('A4 . . . C5 . . . | B4 . . . E4 . . . | A4 . . . C5 . E5 . | D5 . . . B4 . . .',
         'A2 . . . . . . . | E2 . . . . . . . | A2 . . . . . . . | G2 . . . . . . .',
         '----h---'),
        ('F4 . . . A4 . . . | E4 . . . G#4 . . . | A4 . C5 . B4 . A4 . | E4 . . . . . . .',
         'F2 . . . . . . . | E2 . . . . . . . | A2 . . . . . . . | E2 . . . . . . .',
         '----h---'),
    ])

    # Battle: fast, E minor.
    write_mod('bgm_battle', 'placeholder battle', 150, [
        SQUARE,
        ('E5 E5 G5 E5 B4 E5 G5 A5 | B5 . A5 G5 A5 . G5 F#5 | E5 E5 G5 E5 B4 E5 G5 A5 | B5 . D5 . E5 . . .',
         'E2 E3 E2 E3 E2 E3 E2 E3 | D2 D3 D2 D3 D2 D3 D2 D3 | E2 E3 E2 E3 E2 E3 E2 E3 | B2 B2 B2 B2 E2 E3 E2 E3',
         'khshkhsh'),
        ('C5 C5 E5 C5 G4 C5 E5 G5 | D5 D5 F#5 D5 A4 D5 F#5 A5 | B4 B4 D#5 B4 F#4 B4 D#5 F#5 | B5 . A5 . F#5 . D#5 .',
         'C3 C3 C3 C3 C3 C3 C3 C3 | D3 D3 D3 D3 D3 D3 D3 D3 | B2 B2 B2 B2 B2 B2 B2 B2 | B2 B2 B2 B2 B2 B2 D#3 F#3',
         'khshkhss'),
    ])

    # Boss: faster and chromatic, D minor.
    write_mod('bgm_boss', 'placeholder boss', 168, [
        SQUARE,
        ('D5 . D5 F5 . D5 G#5 A5 | D5 . D5 F5 . D5 C5 C#5 | D5 . D5 F5 . D5 G#5 A5 | A#5 A5 G#5 A5 F5 . D5 .',
         'D2 D2 D3 D2 D2 D3 D2 D3 | D2 D2 D3 D2 D2 D3 C3 C#3 | D2 D2 D3 D2 D2 D3 D2 D3 | A#2 A#2 A2 A2 G2 G2 A2 A2',
         'kkshkksh'),
        ('A#4 . A#4 D5 . A#4 E5 F5 | A4 . A4 C#5 . A4 E5 G5 | G4 . G4 A#4 . G4 D5 E5 | A4 C#5 E5 A5 G5 E5 C#5 A4',
         'A#2 A#2 A#3 A#2 A#2 A#3 A#2 A#3 | A2 A2 A3 A2 A2 A3 A2 A3 | G2 G2 G3 G2 G2 G3 G2 G3 | A2 A2 A2 A2 A2 A2 A3 A3',
         'kkshkkss'),
    ])

    # Important event, dark: very slow, D minor. No drums.
    write_mod('bgm_event_dark', 'placeholder dark event', 66, [
        TRIANGLE,
        ('D5 . . . A4 . . . | F4 . . . E4 . . . | D4 . . . F4 . A4 . | G4 . . . . . . .',
         'D3 . . . . . . . | A#2 . . . . . . . | D3 . . . . . . . | G2 . . . . . . .',
         '-'),
        ('A#4 . . . A4 . . . | G4 . . . F4 . . . | E4 . . . G4 . E4 . | D4 . . . . . - .',
         'A#2 . . . . . . . | F2 . . . . . . . | A2 . . . . . . . | D2 . . . . . . .',
         '-'),
    ])

    # Important event, hopeful: calm and rising, G major.
    write_mod('bgm_event_hope', 'placeholder hope event', 92, [
        SQUARE,
        ('G4 . B4 . D5 . . . | E5 . D5 . B4 . . . | C5 . E5 . G5 . . . | F#5 . E5 . D5 . . .',
         'G2 . D3 . G2 . D3 . | E2 . B2 . E2 . B2 . | C3 . G3 . C3 . G3 . | D3 . A2 . D3 . A2 .',
         '--h---h-'),
        ('G5 . . . F#5 . E5 . | D5 . . . B4 . D5 . | C5 . D5 . E5 . F#5 . | G5 . . . . . . .',
         'G2 . D3 . G2 . D3 . | B2 . F#3 . B2 . F#3 . | C3 . G3 . D3 . A3 . | G2 . D3 . G2 . . .',
         '--h---h-'),
    ])

    # Title screen: slow and open, A minor leaning to C. No drums.
    write_mod('bgm_title', 'placeholder title', 84, [
        TRIANGLE,
        ('A4 . . E5 . . D5 C5 | B4 . . G4 . . . . | C5 . . G5 . . F5 E5 | D5 . . . E5 . . .',
         'A2 . E3 . A2 . E3 . | G2 . D3 . G2 . D3 . | F2 . C3 . F2 . C3 . | G2 . D3 . E2 . B2 .',
         '-'),
        ('A5 . . G5 . . E5 . | F5 . . E5 . . C5 . | D5 . E5 . F5 . D5 . | E5 . . . . . - .',
         'F2 . C3 . F2 . C3 . | C3 . G3 . C3 . G3 . | D3 . A2 . D3 . A2 . | E2 . B2 . E2 . . .',
         '-'),
    ])


def main():
    os.makedirs(AUDIO, exist_ok=True)
    gen_sound_effects()
    gen_music()

    for name in sorted(os.listdir(AUDIO)):
        print('%-22s %6d bytes' % (name, os.path.getsize(os.path.join(AUDIO, name))))


if __name__ == '__main__':
    main()
