"""ui/ui_main.cpp's font selection against the Store MP original (native replay, replay.py)."""
import ctypes as C
import random
import struct
import sys

import replay

FUNCTIONS = [("t6mp_msstore", "1403FCD20", "UI_GetFontHandleDefault"), ("t6mp_msstore", "1403FCE40", "UI_GetFontHandle")]
ATOF_IAT = 0x8A7DD8
SOURCES = ["ui/ui_main.cpp", "client/screen_placement.cpp", "universal/dvar.cpp"]


def main(exe):
    with replay.tempdir() as tmp:
        lib = replay.build("ui_fonts_probe.cpp", tmp, SOURCES)
        lib.UiFontsAtof.restype = C.c_void_p
        lib.UiFontsRun.restype = C.c_uint64
        lib.UiFontsRun.argtypes = [C.c_void_p, C.c_uint, C.c_uint, C.c_uint32]
        rng = random.Random(0x3FCD20)
        values = [0, 0x80000000, 0x7FC00000, 0x7F800000, 0xFF800000] + \
                 [struct.unpack("<I", struct.pack("<f", i * 0.125))[0] for i in range(-32, 65)]
        with replay.map_image(exe) as mem:
            replay.set_pointer(mem, ATOF_IAT, lib.UiFontsAtof())
            base = replay.base_of(mem)
            for op, name in enumerate(f[2] for f in FUNCTIONS):
                for n in range(16384):
                    seed, bits = rng.randrange(1 << 28), rng.choice(values)
                    want, got = lib.UiFontsRun(base, op, seed, bits), lib.UiFontsRun(None, op, seed, bits)
                    assert want == got, (name, n, hex(seed), hex(bits), hex(want), hex(got))
                print(f"{name}: 16384 inputs identical", flush=True)


if __name__ == "__main__":
    main(sys.argv[1])
