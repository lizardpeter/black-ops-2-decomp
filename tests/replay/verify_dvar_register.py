"""universal/dvar.cpp's _Dvar_RegisterFloat against the Store MP original (native replay, replay.py)."""
import ctypes as C
import random
import sys

import replay

FUNCTIONS = [("t6mp_msstore", "14046C650", "_Dvar_RegisterFloat")]
HOOKS = [0x46B8B0, 0x46C1A0, 0x46BF10]  # Dvar_FindMalleableVar, Dvar_RegisterNew, Dvar_Reregister


def main(exe):
    with replay.tempdir() as tmp:
        lib = replay.build("dvar_register_probe.cpp", tmp, ["universal/dvar.cpp"])
        lib.DvarRegisterHook.restype = C.c_void_p
        lib.DvarRegisterHook.argtypes = [C.c_uint]
        lib.DvarRegisterSize.restype = C.c_uint
        lib.DvarRegisterRun.argtypes = [C.c_void_p, C.c_uint, C.c_float, C.c_float, C.c_float, C.c_void_p]
        a, b = (C.create_string_buffer(lib.DvarRegisterSize()) for _ in range(2))
        rng = random.Random(0x46C650)
        with replay.map_image(exe) as mem:
            for i, rva in enumerate(HOOKS):
                replay.hook(mem, rva, lib.DvarRegisterHook(i))
            base = replay.base_of(mem)
            for n in range(4096):
                seed = rng.getrandbits(32)
                v = [rng.choice([0.0, -0.0, 1.0, 0.2, 2.0, 1e30, float("inf"), float("nan"), rng.uniform(-9, 9)])
                     for _ in range(3)]
                lib.DvarRegisterRun(base, seed, *v, a)
                lib.DvarRegisterRun(None, seed, *v, b)
                assert a.raw == b.raw, (n, hex(seed), v)
            print("_Dvar_RegisterFloat: 4096 inputs identical", flush=True)


if __name__ == "__main__":
    main(sys.argv[1])
