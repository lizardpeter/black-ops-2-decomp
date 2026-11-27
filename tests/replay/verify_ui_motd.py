"""ui/ui_motd.cpp against the Store MP original (native replay, replay.py): MOTD_Parse on generated MOTD files, with
MOTD_ParseDataEntry, the value parsers and what MOTD_Parse inlines (universal/mem_largelocal.cpp's LargeLocal, the
JSMN session, qcommon/cmd.cpp's Cmd_AddCommand)."""
import ctypes as C
import sys

import replay

PARSERS = [("1404098F0", "MOTD_ParseVersion"), ("140409920", "MOTD_ParseMOTDVersion"),
           ("140409960", "MOTD_ParseSingleView"), ("1404099A0", "MOTD_ParseExpiration"),
           ("1404099E0", "MOTD_ParseImageCategory"), ("140409A20", "MOTD_ParseTitle"),
           ("140409A80", "MOTD_ParseMessage"), ("140409AE0", "MOTD_ParseImage"), ("140409B40", "MOTD_ParseAction"),
           ("140409BA0", "MOTD_ParseActionContext"), ("140409C00", "MOTD_ParseActionString"),
           ("140409C60", "MOTD_ParseBannerTitle"), ("140409CC0", "MOTD_ParseBannerMessage"),
           ("140409D20", "MOTD_ParseBannerImage"), ("140409D80", "MOTD_ParseBannerAction"),
           ("140409DE0", "MOTD_ParseBannerActionContext"), ("140409E40", "MOTD_ParseBannerActionString"),
           ("140409EA0", "MOTD_ParseEliteImage"), ("140409F00", "MOTD_ParseFounderImage")]
FUNCTIONS = [("t6mp_msstore", va, n) for va, n in PARSERS + [("140409F60", "MOTD_ParseDataEntry"),
                                                             ("14040A860", "MOTD_Parse")]]
SOURCES = ["ui/ui_motd.cpp", "json/json.cpp", "universal/mem_largelocal.cpp", "qcommon/cmd.cpp",
           "qcommon/com_clients.cpp"]
HOOKS = [0x492660, 0x6623F0, 0x69DA90, 0x662570]  # lua_getfield, lua_isfunction, lua_pcall, lua_toboolean
# Enter/LeaveCriticalSection, GetCurrentThreadId, atoi, memset, strcmp
IAT = [(0x8A72F0, 4), (0x8A72F8, 5), (0x8A7258, 6), (0x8A7DC0, 7), (0x8A79E0, 8), (0x8A8248, 9)]
CASES = 6000


def main(exe):
    with replay.tempdir() as tmp:
        lib = replay.build("ui_motd_probe.cpp", tmp, SOURCES)
        lib.MotdHook.restype = C.c_void_p
        lib.MotdHook.argtypes = [C.c_uint]
        lib.MotdOutSize.restype = C.c_uint
        lib.MotdRun.restype = C.c_uint
        lib.MotdRun.argtypes = [C.c_void_p, C.c_uint, C.c_void_p]
        lib.MotdInit.argtypes = [C.c_void_p]
        a, b = (C.create_string_buffer(lib.MotdOutSize()) for _ in range(2))
        with replay.map_image(exe) as mem:
            for i, rva in enumerate(HOOKS):
                replay.hook(mem, rva, lib.MotdHook(i))
            for rva, i in IAT:
                replay.set_pointer(mem, rva, lib.MotdHook(i))
            base = replay.base_of(mem)
            assert lib.MotdInit(C.c_void_p(base)) == 0, "arch_prctl(ARCH_SET_GS) failed"
            parsed = valid = banners = 0
            for seed in range(CASES):
                n = lib.MotdRun(base, seed, a)
                m = lib.MotdRun(None, seed, b)
                assert n == m and a.raw[:n] == b.raw[:n], (seed, n, m, next(
                    (i for i in range(min(n, m)) if a.raw[i] != b.raw[i]), None))
                last = lib.MotdLast()
                parsed, valid, banners = parsed + (last & 1), valid + (last >> 1 & 1), banners + (last >> 2 & 1)
            assert min(parsed, valid, banners) > CASES // 50, f"{parsed} parsed, {valid} valid, {banners} banners: no test"
            print(f"MOTD_Parse: {CASES} files identical ({parsed} parsed, {valid} with a message, {banners} with a "
                  "banner)", flush=True)


if __name__ == "__main__":
    main(sys.argv[1])
