"""gfx_d3d/r_font.cpp against the Store MP original (native replay, replay.py)."""
import ctypes as C
import random
import sys

import replay

FUNCTIONS = [("t6mp_msstore", "14071A6E0", "R_GetCharacterGlyph"), ("t6mp_msstore", "14071A780", "R_RegisterFont"),
             ("t6mp_msstore", "14071A7A0", "R_TextWidth")]
DB_FIND_XASSET_HEADER, R_GET_FONT_ICON_ENTRY_INFO = 0x212150, 0x723450


def main(exe):
    with replay.tempdir() as tmp:
        lib = replay.build("r_font_probe.cpp", tmp, ["gfx_d3d/r_font.cpp", "stringed/stringed_hooks.cpp"])
        lib.RFontHook.restype = C.c_void_p
        lib.RFontHook.argtypes = [C.c_uint]
        lib.RFontCaptureSize.restype = C.c_uint
        lib.RFontRun.argtypes = [C.c_void_p, C.c_uint, C.c_uint, C.c_void_p]
        a, b = (C.create_string_buffer(lib.RFontCaptureSize()) for _ in range(2))
        rng = random.Random(0x71A6E0)
        with replay.map_image(exe) as mem:
            replay.hook(mem, DB_FIND_XASSET_HEADER, lib.RFontHook(0))
            replay.hook(mem, R_GET_FONT_ICON_ENTRY_INFO, lib.RFontHook(1))
            base = replay.base_of(mem)
            for op, name in enumerate(f[2] for f in FUNCTIONS):
                for n in range(8192):
                    seed = rng.randrange(1 << 25)
                    lib.RFontRun(base, op, seed, a)
                    lib.RFontRun(None, op, seed, b)
                    assert a.raw == b.raw, (name, n, hex(seed))
                print(f"{name}: 8192 inputs identical", flush=True)


if __name__ == "__main__":
    main(sys.argv[1])
