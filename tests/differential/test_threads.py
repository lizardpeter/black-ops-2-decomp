"""qcommon/threads.cpp, universal/com_workercmds.cpp and gfx_d3d/r_rendercmds.cpp: which thread is which, the front
end's job check, and the hand-off of a frame to the render thread.

- Server: Sys_IsMainThread / Sys_IsRenderThread / Sys_IsServerThread (the thread id cached in TLS, GetCurrentThreadId
  modelled), R_FinishedFrontendWorkerCmds (the original jqPoll runs over group counts each input sets).
- Store: Sys_WaitRenderer (the dedicated test faked, the wait's results scripted) and R_ToggleSmpFrameCmd (the
  renderer wait, jqPoll and R_ToggleSmpFrame faked and observed; the Win32 calls logged).
The Win32 calls log to the compared OBJECTS region.
"""
import random
import struct

from unicorn import x86_const as x86

import symbols
from harness import OBJECTS, TLS_BLOCK

SERVER, STORE = "CoDMPServer_PC", "t6mp_msstore"
STATE = {}
LOG = OBJECTS + 0x1F0000  # count, then (tag, value) pairs
DATA = 0x38000000         # a back-end data block for R_ToggleSmpFrameCmd (Store offsets reach 0x4BEA50)
VB = OBJECTS + 0x1000     # its skinned cache's GfxVertexBufferState

# the thread id's TLS offset (g_currentThreadId), threadId, renderCompletedEvent / backendEvent / smpData
TLS_THREAD_ID = {SERVER: 0x54, STORE: 0x3C}
CONTEXTS = {"main": 0, "backend": 1, "server": 10}


def va(build, sym):
    hit = symbols.resolve(build, sym)
    assert hit, f"{sym} not identified in {build}"
    return hit[0]


def static(build, name, source="qcommon/threads.cpp"):
    return va(build, f"t6s@{source}@@{name}")


def x64():
    return STATE["build"] == STORE


def arg(mu, i):
    if x64():
        return mu.reg_read((x86.UC_X86_REG_RCX, x86.UC_X86_REG_RDX, x86.UC_X86_REG_R8, x86.UC_X86_REG_R9)[i])
    return struct.unpack("<I", bytes(mu.mem_read(mu.reg_read(x86.UC_X86_REG_ESP) + 4 + 4 * i, 4)))[0]


def result(mu, value):
    mu.reg_write(x86.UC_X86_REG_RAX if x64() else x86.UC_X86_REG_EAX, value & 0xFFFFFFFF)


def log(m, tag, value=0):
    n = struct.unpack("<I", bytes(m.mu.mem_read(LOG, 4)))[0]
    m.program_write(LOG + 8 + 16 * n, struct.pack("<QQ", tag, value))
    m.program_write(LOG, struct.pack("<I", n + 1))


def get_thread_id(m, mu):
    log(m, 0x7D)
    result(mu, STATE["tid"])


def logged(tag, ret=1):
    def run(m, mu):
        log(m, tag, arg(mu, 0))
        result(mu, ret)
    return run


def wait_single(m, mu):
    log(m, 0x3A17, arg(mu, 0))
    log(m, 0x3A18, arg(mu, 1) & 0xFFFFFFFF)
    script = STATE["waits"]
    if not script:  # the original would wait on: both runs stop here
        m.end_call()
        return
    result(mu, script.pop(0))


IMPORTS = {"kernel32.dll!GetCurrentThreadId": (get_thread_id, 0),
           "kernel32.dll!WaitForSingleObject": (wait_single, 2),
           "kernel32.dll!Sleep": (logged(0x51EE, 0), 1),
           "kernel32.dll!EnterCriticalSection": (logged(0xCE), 1),
           "kernel32.dll!LeaveCriticalSection": (logged(0xCF), 1),
           "kernel32.dll!ResetEvent": (logged(0xE5), 1), "kernel32.dll!SetEvent": (logged(0xE7), 1)}


def threads(build, tid, cached, ids):
    """The calling thread's id (GetCurrentThreadId's), its cached copy in TLS, and threadId's entries."""
    def run(w):
        STATE.update(build=build, tid=tid, waits=[])
        w.write(TLS_BLOCK + TLS_THREAD_ID[build], struct.pack("<I", cached))
        base = static(build, "threadId")
        for context, value in ids.items():
            w.write(base + 4 * CONTEXTS[context], struct.pack("<I", value))
    return run


IDS = [{"main": 0x111, "backend": 0x222, "server": 0x333}, {"main": 0x222, "backend": 0x111, "server": 0x111},
       {"main": 0, "backend": 0, "server": 0}, {"main": 0x333, "backend": 0x333, "server": 0x222}]
THREAD_INPUTS = [{"args": [], "setup": threads(SERVER, tid, cached, ids)}
                 for tid in (0, 0x111, 0x222, 0x333) for cached in (0, 0x111, 0x333) for ids in IDS]

CASES = [{"name": n, "ret": "bool", "builds": [SERVER], "imports": IMPORTS, "inputs": THREAD_INPUTS}
         for n in ("Sys_IsMainThread", "Sys_IsRenderThread", "Sys_IsServerThread")]


# -- R_FinishedFrontendWorkerCmds: the 20 groups in the order it polls them
GROUPS = ["fx_spawn", "fx_update_spot", "fx_update", "fx_update_remaining", "r_add_staticmodel",
          "r_add_staticmodel_sun", "r_add_bsp", "r_add_bsp_sun", "r_dpvs_static", "r_dpvs_sceneent", "r_dpvs_dynmodel",
          "r_dpvs_dynbrush", None, "dobj_skel", "dobj_skin", "fx_draw", "fx_marks_draw", "r_model_skin", "nuge_physics",
          "r_add_draw_calls"]  # None: r_sceneentGroup, a group with no module


def group_va(build, name):
    if name is None:
        return va(build, "?r_sceneentGroup@@3UjqBatchGroup@@A")
    return va(build, f"?{name}Module@@3UjqModule@@A") + (0x18 if build == STORE else 0x10)


def groups(build, busy):
    """busy: {group index: (queued, executing)}; every other group idle."""
    def run(w):
        STATE.update(build=build)
        for i, name in enumerate(GROUPS):
            w.write(group_va(build, name), struct.pack("<ii", *busy.get(i, (0, 0))))
    return run


def group_inputs(build):
    r = random.Random(0x20)
    out = [{"args": [], "setup": groups(build, {})}]
    for i in range(len(GROUPS)):
        out += [{"args": [], "setup": groups(build, {i: q})} for q in ((1, 0), (0, 1), (-1, 0), (0, 0x10000))]
    for _ in range(20):
        out.append({"args": [], "setup": groups(build, {i: (r.randrange(3), r.randrange(3))
                                                          for i in r.sample(range(len(GROUPS)), r.randrange(1, 6))})})
    return out


CASES.append({"name": "R_FinishedFrontendWorkerCmds", "ret": "int", "builds": [SERVER],
              "inputs": group_inputs(SERVER)})


# -- Sys_WaitRenderer (Store): the dedicated test faked; the wait's results scripted (the script running out stops
# both runs, as a wait that never returns)
def wait_setup(dedicated, tid, cached, main, waits, event=0xE0E0):
    def run(w):
        threads(STORE, tid, cached, {"main": main, "backend": 0x222})(w)
        STATE.update(dedicated=dedicated, waits=list(waits))
        w.write(static(STORE, "renderCompletedEvent"), struct.pack("<Q", event))
    return run


def wait_inputs():
    out = []
    for dedicated in (0, 1):
        for tid, cached, main in ((0x111, 0, 0x111), (0x111, 0x111, 0x111), (0x222, 0, 0x111), (0x111, 0x222, 0x111),
                                  (0, 0, 0), (0x111, 0, 0)):
            for waits in ([0], [0x102, 0], [0x102, 0x102, 0x102, 0], [0xFFFFFFFF, 0x80, 0], [0x102] * 3):
                out.append({"args": [], "setup": wait_setup(dedicated, tid, cached, main, waits)})
    return out


CASES.append({"name": "Sys_WaitRenderer", "ret": "int", "builds": [STORE], "imports": IMPORTS,
              "fakes": {"Com_IsDedicatedServer": lambda mu: STATE["dedicated"]}, "observe": {"Com_IsDedicatedServer": ""},
              "inputs": wait_inputs()})


# -- R_ToggleSmpFrameCmd (Store): the renderer wait, jqPoll (busy per a script of results) and R_ToggleSmpFrame (it
# swaps frontEndDataOut, so the frame handed over must be the one read before it) are faked and observed
def poll(mu):
    script = STATE["polls"]
    return script.pop(0) if script else 0


def toggle(mu):
    STATE["m"].program_write(va(STORE, "?frontEndDataOut@@3PEAUGfxBackEndData@@EA"), struct.pack("<Q", DATA + 0x10))
    return 0


def toggle_setup(p):
    def run(w):
        STATE.update(build=STORE, polls=list(p["polls"]), waits=[], m=w.m, tid=0x111)
        try:
            w.m.mu.mem_map(DATA, 0x500000)
        except Exception:
            pass
        w.write(va(STORE, "?frontEndDataOut@@3PEAUGfxBackEndData@@EA"), struct.pack("<Q", DATA))
        w.write(DATA + 0x4BE9C0, struct.pack("<Q", VB))
        w.write(DATA + 0x4BE9C8, struct.pack("<i", p["allocated"]))
        w.write(DATA + 0x4BEA50, struct.pack("<I", p["view"]))
        w.write(VB, struct.pack("<ii", p["used"], p["total"]))
        w.write(va(STORE, "?store_142C3E760@@3PEAXEA"), struct.pack("<Q", 0xDEADBEEF12345678))
        w.write(static(STORE, "smpData"), struct.pack("<Q", 0x5150))
        for name, h in (("renderCompletedEvent", 0xE0E0), ("backendEvent", 0xBE00)):
            w.write(static(STORE, name), struct.pack("<Q", h))
        w.write(static(STORE, "backendEvent") + 8, struct.pack("<Q", 0xBE01))
    return run


def toggle_inputs():
    r = random.Random(0x14071B880)
    out = []
    for _ in range(60):
        polls = [r.choice([0, 0, 0, 1]) for _ in range(r.randrange(0, 70))]
        out.append({"args": [r.choice([0, 1, 2, 0xFFFFFFFF])],
                    "setup": toggle_setup({"polls": polls, "allocated": r.choice([0, 1, 1000, 0x7FFFFFFF, -5]),
                                           "total": r.choice([1, 3, 4096, 0x10000, -7]), "used": r.randrange(100),
                                           "view": r.randrange(4)})})
    out.append({"args": [0], "setup": toggle_setup({"polls": [], "allocated": 3, "total": 0, "used": 0, "view": 0})})
    return out


CASES.append({"name": "R_ToggleSmpFrameCmd", "ret": "void", "builds": [STORE], "imports": IMPORTS,
              "fakes": {"Sys_WaitRenderer": lambda mu: 0, "jqPoll": poll, "R_ToggleSmpFrame": toggle},
              "observe": {"Sys_WaitRenderer": "", "jqPoll": "q", "R_ToggleSmpFrame": ""},
              "inputs": toggle_inputs()})
