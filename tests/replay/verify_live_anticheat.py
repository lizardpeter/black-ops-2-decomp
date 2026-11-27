"""live/live_anticheat.cpp's hashing against the Store MP original (native replay, replay.py)."""
import ctypes as C
import random
import sys

import replay

FUNCTIONS = [("t6mp_msstore", "140576490", "LiveAntiCheat_StartHash"),
             ("t6mp_msstore", "140576540", "LiveAntiCheat_ContinueHash")]


def main(exe):
    with replay.tempdir() as tmp:
        lib = replay.build("live_anticheat_probe.cpp", tmp, ["live/live_anticheat.cpp"])
        assert lib.AntiCheatInit(), "no memory below 4 GB"
        lib.AntiCheatRun.argtypes = [C.c_void_p, C.c_uint, C.c_uint, C.c_void_p]
        size = 8 + 8 + 16 * 0x20
        a, b = C.create_string_buffer(size), C.create_string_buffer(size)
        rng = random.Random(0x576540)
        with replay.map_image(exe) as mem:
            base = replay.base_of(mem)
            for op, (_, _, name) in enumerate(FUNCTIONS):
                for n in range(8192):
                    seed = rng.getrandbits(32)
                    lib.AntiCheatRun(base, op, seed, a)
                    lib.AntiCheatRun(None, op, seed, b)
                    assert a.raw == b.raw, (name, n, hex(seed))
                print(f"{name}: 8192 inputs identical", flush=True)


if __name__ == "__main__":
    main(sys.argv[1])
