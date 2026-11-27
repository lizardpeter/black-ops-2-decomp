"""gfx_d3d/r_font.cpp and stringed/stringed_hooks.cpp on the servers (the Store build: tests/replay/verify_r_font.py)."""

import random
import struct

from harness import OBJECTS
import test_win_input as tw

SERVER = ["CoDMPServer_PC"]
FONT, GLYPHS = OBJECTS, OBJECTS + 0x100
rng = random.Random(0xA28200)


def font_setup(count, height=20):
    def run(w):
        tw.STATE.clear()
        w.write(FONT, struct.pack("<IiiiiIIII", 0, height, 0, count, 0, 0, 0, GLYPHS, 0))
        glyphs = b""
        for i in range(160):
            letter = i + 32 if i < 96 else 0x80 + (i - 96) * 7
            glyphs += struct.pack("<HbbBBBx4f", letter, 0, 0, (i * 37 + 11) & 0xFF, 0, 0, 0, 0, 0, 0)
        w.write(GLYPHS, glyphs)
    return run


def icon_info(mu):
    """R_GetFontIconEntryInfo: no material, a width of 1.25 heights."""
    tw.STATE["x64"] = False
    for i, v in ((3, b"\0\0\0\0"), (4, struct.pack("<f", 1.25)), (5, struct.pack("<f", 0.5))):
        mu.mem_write(tw.arg(mu, i), v)


TEXTS = [b"", b"abc", b"^1red^7white", b"^C2x", b"^Lq", b"^F", b"^^", b"^X", b"a\nbcd\r", b"\x14\x15z",
         b"\xc2\x80\xe1\x80\x80", b"^Bicon^z", b"^B^", b"\xff\xfe", b"^C^", b"x" * 40]

CASES = [
    {"name": "SEH_DecodeLetter", "ret": "uint", "builds": SERVER,
     "inputs": [[a, b, c, bytes(4)] for a in (0, 0x41, 0x7F, 0x80, 0xC2, 0xDF, 0xE1, 0xEF, 0xF0, 0xFF)
                for b in (0, 0x80, 0xBF, 0x41) for c in (0x80, 0x00)]},
    {"name": "SEH_ReadCharFromString", "ret": "uint", "builds": SERVER,
     "inputs": [[t + b"\0\0\0"] for t in TEXTS if t]},
    {"name": "R_GetCharacterGlyph", "ret": "uint", "builds": SERVER,
     "inputs": [{"args": [FONT, letter], "setup": font_setup(count)}
                for count in (96, 97, 120, 160) for letter in (32, 46, 65, 127, 0x80, 0x87, 0x8E, 0x100, 0x1234, 0xFFFFFFFF)]},
    {"name": "R_TextWidth", "ret": "int", "builds": SERVER,
     "fakes": {"R_GetFontIconEntryInfo": icon_info},
     "observe": {"R_GetFontIconEntryInfo": "isi"},
     "inputs": [{"args": [0, t + b"\0", maxChars, FONT], "setup": font_setup(120, rng.randrange(1, 64))}
                for t in TEXTS for maxChars in (0, 3, 100)]},
    {"name": "R_RegisterFont", "ret": "uint", "builds": SERVER,
     "fakes": {"R_RegisterFont_FastFile": lambda mu: 0x1234, "R_RegisterFont_LoadObj": lambda mu: 0x5678,
               "?Dvar_GetBool@@YA_NPBUdvar_t@@@Z": lambda mu: 1},
     "inputs": [[b"fonts/normalfont\0", 0]]},
    {"name": "R_RegisterFont_FastFile", "ret": "uint", "builds": SERVER,
     "fakes": {"DB_FindXAssetHeader": lambda mu: 0x4321}, "observe": {"DB_FindXAssetHeader": "isii"},
     "inputs": [[b"fonts/normalfont\0", 0], [b"fonts/bigfont\0", 3]]},
]
