"""client/screen_placement.cpp and the Store's aim assist screen fractions against the Store MP original."""
import ctypes as C
import random
import sys

import replay

FUNCTIONS = [("t6mp_msstore", "1401F65E0", "ScrPlace_ApplyRect"),
             ("t6mp_msstore", "1400DDB40", "ScrPlace_VirtualFromReal_Width"),
             ("t6mp_msstore", "1400D7110", "ScrPlace_VirtualFromReal_Height"),
             ("t6mp_msstore", "1400A9AB0", "AimAssist_VirtualToScreenX"),
             ("t6mp_msstore", "1400A9B20", "AimAssist_VirtualToScreenY")]
SOURCES = ["client/screen_placement.cpp", "aim_assist/aim_assist.cpp", "qcommon/com_clients.cpp"]


def main(exe):
    with replay.tempdir() as tmp:
        lib = replay.build("screen_placement_probe.cpp", tmp, SOURCES)
        lib.ScrPlaceRun.argtypes = [C.c_void_p, C.c_uint, C.c_uint, C.c_void_p]
        a, b = C.create_string_buffer(20), C.create_string_buffer(20)
        rng = random.Random(0x1F65E0)
        with replay.map_image(exe) as mem:
            base = replay.base_of(mem)
            for op, (_, _, name) in enumerate(FUNCTIONS):
                for n in range(16384):
                    seed = rng.getrandbits(32)
                    lib.ScrPlaceRun(base, op, seed, a)
                    lib.ScrPlaceRun(None, op, seed, b)
                    assert a.raw == b.raw, (name, n, hex(seed), a.raw.hex(), b.raw.hex())
                print(f"{name}: 16384 inputs identical", flush=True)


if __name__ == "__main__":
    main(sys.argv[1])
