"""json/json.cpp against the Store MP original (native replay, replay.py)."""
import ctypes as C
import sys

import replay

FUNCTIONS = [("t6mp_msstore", "14034C640", "jsmn_parse"), ("t6mp_msstore", "14034CBB0", "JSMN_GetToken"),
             ("t6mp_msstore", "14034CD10", "JSMN_GetLocalizedString")]
TRANSLATE, ATOI_IAT = 0x3FF450, 0x8A7DC0  # UI_SafeTranslateString; atoi's import slot
CASES = 4000


def main(exe):
    with replay.tempdir() as tmp:
        lib = replay.build("json_probe.cpp", tmp, ["json/json.cpp"])
        lib.JsonHook.restype = C.c_void_p
        lib.JsonHook.argtypes = [C.c_uint]
        lib.JsonOutSize.restype = C.c_uint
        lib.JsonRun.restype = C.c_uint
        lib.JsonRun.argtypes = [C.c_void_p, C.c_uint, C.c_uint, C.c_void_p]
        a, b = (C.create_string_buffer(lib.JsonOutSize()) for _ in range(2))
        with replay.map_image(exe) as mem:
            replay.hook(mem, TRANSLATE, lib.JsonHook(0))
            replay.set_pointer(mem, ATOI_IAT, lib.JsonHook(1))
            base = replay.base_of(mem)
            lib.JsonInit(C.c_void_p(base))
            for op, name in enumerate(["jsmn_parse", "JSMN_GetToken", "JSMN_GetLocalizedString"]):
                for seed in range(CASES):
                    n = lib.JsonRun(base, op, seed, a)
                    m = lib.JsonRun(None, op, seed, b)
                    assert n == m and a.raw[:n] == b.raw[:n], (name, seed, n, m)
                print(f"{name}: {CASES} inputs identical", flush=True)


if __name__ == "__main__":
    main(sys.argv[1])
