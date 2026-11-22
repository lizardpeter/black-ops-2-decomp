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
