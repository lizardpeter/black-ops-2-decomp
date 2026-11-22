"""universal/win_common.cpp and qcommon/threads.cpp: the critical sections and the quit event (server; the Store build
inlines them). The Win32 calls are modelled: each logs its argument (the critical section's or event's address) to
the harness's compared OBJECTS region.
"""
import struct

from unicorn import x86_const as x86

from harness import OBJECTS

SERVER = ["CoDMPServer_PC"]
LOG = OBJECTS + 0x1F0000


def logged(tag):
    def run(m, mu):
        arg = struct.unpack("<I", bytes(mu.mem_read(mu.reg_read(x86.UC_X86_REG_ESP) + 4, 4)))[0]
        n = struct.unpack("<I", bytes(mu.mem_read(LOG, 4)))[0]
        m.program_write(LOG + 4 + 8 * n, struct.pack("<II", tag, arg))
        m.program_write(LOG, struct.pack("<I", n + 1))
        mu.reg_write(x86.UC_X86_REG_EAX, 1)
    return run


IMPORTS = {"kernel32.dll!EnterCriticalSection": (logged(0xCE), 1),
           "kernel32.dll!LeaveCriticalSection": (logged(0xCF), 1), "kernel32.dll!SetEvent": (logged(0xE7), 1)}
ASSERT = {"fakes": {"Assert_MyHandler": lambda mu: 1}, "observe": {"Assert_MyHandler": "siissii"}}


def quit_event(handle):
    def run(w):
        w.write(0x0780031C, struct.pack("<I", handle))
    return run


SECTIONS = [0, 1, 0x21, 0x25, 0x4D, 0x4E, 0x80, -1]   # 0x4E is CRITSECT_COUNT: the bounds assert, then on regardless

CASES = [
    {"name": "Sys_EnterCriticalSection", "ret": "void", "builds": SERVER, "imports": IMPORTS, **ASSERT,
     "inputs": [[s] for s in SECTIONS]},
    {"name": "Sys_LeaveCriticalSection", "ret": "void", "builds": SERVER, "imports": IMPORTS, **ASSERT,
     "inputs": [[s] for s in SECTIONS]},
    {"name": "Sys_SetWin32QuitEvent", "ret": "void", "builds": SERVER, "imports": IMPORTS,
     "inputs": [{"args": [], "setup": quit_event(h)} for h in (0, 0x1234, 0xFFFFFFFF)]},
]


# -- per-thread values (qcommon/threads.cpp): g_dwTlsIndex, the thread's value table, at TLS offset 0x50
from harness import TLS_BLOCK  # noqa: E402

VALUES = OBJECTS + 0x10000


def values(table):
    def run(w):
        w.write(TLS_BLOCK + 0x50, struct.pack("<I", VALUES))
        w.write(VALUES, struct.pack(f"<{len(table)}I", *table))
    return run


TABLE = [0x11111111, 0x22222222, 0x33333333, 0, 0x55555555, 0xFFFFFFFF]
CASES += [
    {"name": "Sys_GetValue", "ret": "int", "builds": SERVER,
     "inputs": [{"args": [i], "setup": values(TABLE)} for i in range(len(TABLE))]},
    {"name": "Sys_SetValue", "ret": "void", "builds": SERVER,
     "inputs": [{"args": [i, v], "setup": values(TABLE)} for i in range(len(TABLE)) for v in (0, 0xABCDEF01)]},
]


# -- Sys_TryEnterCriticalSection: TryEnterCriticalSection logs its argument and returns the scripted result
TRY = {}


def try_enter(m, mu):
    logged(0x7E)(m, mu)
    mu.reg_write(x86.UC_X86_REG_EAX, TRY["result"])


def try_result(v):
    def run(w):
        TRY["result"] = v
    return run


# -- Sys_Milliseconds: timeGetTime returns a scripted clock that steps by 3 each call
CLOCK = {}


def time_get_time(m, mu):
    CLOCK["now"] = (CLOCK["now"] + 3) & 0xFFFFFFFF
    mu.reg_write(x86.UC_X86_REG_EAX, CLOCK["now"])


def clock(now, base, initialized):
    def run(w):
        CLOCK["now"] = now
        w.write(0x07ACCFC0, struct.pack("<i", base))         # sys_timeBase
        w.write(0x07ACCFC8, struct.pack("<i", initialized))  # Sys_Milliseconds' initialized
    return run


CASES += [
    {"name": "Sys_TryEnterCriticalSection", "ret": "bool", "builds": SERVER,
     "imports": dict(IMPORTS, **{"kernel32.dll!TryEnterCriticalSection": (try_enter, 1)}), **ASSERT,
     "inputs": [{"args": [s], "setup": try_result(r)} for s in SECTIONS for r in (0, 1, 2)]},
    {"name": "Sys_Milliseconds", "ret": "int", "builds": SERVER,
     "imports": {"winmm.dll!timeGetTime": (time_get_time, 0)},
     "inputs": [{"args": [], "setup": clock(now, base, init)} for now in (0, 100, 0xFFFFFFF0, 0x7FFFFFFF)
                for base in (0, 50, -10, 0x7FFFFFFF) for init in (0, 1)]},
]
