"""qcommon/common.cpp: Com_Frame_Try_Block_Function on the Store build (1403A4010), and the server functions it inlines:
Sys_Milliseconds (win32/win_shared.cpp), Sys_TryEnterCriticalSection (universal/win_common.cpp), GPad_StopRumbles
(win32/win_gamepad.cpp).

Every callee of the frame is faked and observed; the clock (timeGetTime) is modelled: it moves only when Sleep is
called, or by a scripted step per call. Dvars are scratch dvar_ts; the frame's globals start from scripted values.
"""
import random
import struct

from unicorn import x86_const as x86

from harness import OBJECTS

SERVER, STORE = "CoDMPServer_PC", "t6mp_msstore"
F = {}                        # the current input's parameters and machine
LOG = OBJECTS + 0x1F0000      # import effects: count, then (tag, value) pairs
DVARS = OBJECTS + 0x1E0000
SERVER_INFO = OBJECTS + 0x1D0000
TASK = OBJECTS + 0x1C0000     # what operator new hands out
QUEUE = 0x1441AFEF0
G = {"com_maxfps": 0x141F03820, "com_dedicated": 0x141F04908, "com_statmon": 0x141F04920, "cl_paused": 0x141F04888,
     "sv_running": 0x141F048F0, "com_frameTime": 0x141F04894, "com_lastFrameIndex": 0x143F81DEC,
     "com_lastFrameTime": 0x143F81DE0, "com_lastFrameTime_base": 0x143F81E30, "sys_timeBase": 0x1421E448C,
     "initialized": 0x14449E974, "serverInfo": 0x140E53B70, "clientGameStates": None, "primary": 0x140A53D88,
     "cls": 0x142CB7C80, "demo": 0x1425A3280, "store_140E61798": 0x140E61798, "store_14413FEED": 0x14413FEED,
     "store_14413FEE4": 0x14413FEE4, "store_14413FEE8": 0x14413FEE8, "store_143ED0528": 0x143ED0528,
     "store_143E039FF": 0x143E039FF, "store_143ED03D4": 0x143ED03D4, "store_143ED0578": 0x143ED0578,
     "store_143E33FED": 0x143E33FED, "store_143C0A758": 0x143C0A758, "store_142155E08": 0x142155E08,
     "store_1444BA284": 0x1444BA284, "store_140E5CB94": 0x140E5CB94, "store_140A5AAAB": 0x140A5AAAB,
     "store_1427F1B58": 0x1427F1B58, "s_gamePads": 0x14420D5A0, "store_141EF8518": 0x141EF8518}


def arg(mu, i):
    if i < 4:
        return mu.reg_read((x86.UC_X86_REG_RCX, x86.UC_X86_REG_RDX, x86.UC_X86_REG_R8, x86.UC_X86_REG_R9)[i])
    return struct.unpack("<Q", bytes(mu.mem_read(mu.reg_read(x86.UC_X86_REG_RSP) + 8 + 8 * i, 8)))[0]


def result(mu, v):
    mu.reg_write(x86.UC_X86_REG_RAX, v & 0xFFFFFFFFFFFFFFFF)


def log(m, tag, value):
    n = struct.unpack("<I", bytes(m.mu.mem_read(LOG, 4)))[0]
    m.program_write(LOG + 8 + 16 * n, struct.pack("<QQ", tag, value & 0xFFFFFFFFFFFFFFFF))
    m.program_write(LOG, struct.pack("<I", n + 1))


# -- imports: the clock and the rest
def time_get_time(m, mu):
    F["clock"] += F["p"]["step"]
    result(mu, F["clock"] & 0xFFFFFFFF)


def sleep(m, mu):
    ms = arg(mu, 0) & 0xFFFFFFFF
    log(m, 0x51EE, ms)
    F["sleeps"] += 1
    F["clock"] += F["p"]["sleepMs"] if ms else F["p"]["yield"]
    if not ms and F["busy"]:  # the queued calls' busy flag drops after a few yields
        F["busy"] -= 1
        if not F["busy"]:
            m.program_write(G["store_14413FEE4"], struct.pack("<i", 0))
    if F["sleeps"] > 400:  # a wait the script can't end: stop (both runs equally)
        m.end_call()


def try_enter(m, mu):
    log(m, 0x7E, arg(mu, 0))
    result(mu, F["p"]["tryEnter"])


def leave(m, mu):
    log(m, 0x7F, arg(mu, 0))


def xinput_set_state(m, mu):
    log(m, 0x5E7, arg(mu, 0) & 0xFFFFFFFF)
    log(m, 0x5E8, struct.unpack("<I", bytes(mu.mem_read(arg(mu, 1), 4)))[0])
    result(mu, 0)


IMPORTS = {"winmm.dll!timeGetTime": (time_get_time, 0), "kernel32.dll!Sleep": (sleep, 1),
           "kernel32.dll!TryEnterCriticalSection": (try_enter, 1),
           "kernel32.dll!LeaveCriticalSection": (leave, 1), "xinput1_4.dll!#3": (xinput_set_state, 2)}


# -- fakes
def event_loop(mu):
    F["clock"] += F["p"]["eventLoopMs"]


def poll(mu):
    F["polls"] += 1
    return 1 if F["polls"] <= F["p"]["polls"] else 0


def new(mu):
    return TASK


def queued_call(mu):
    F["m"].observed.append(("queued", arg(mu, 0)))
    F["clock"] += F["p"]["callMs"]


FAKES = {"sub_1403A2FF0": lambda mu: None, "Com_EventLoop": event_loop, "Com_IsDedicatedServer": lambda mu: F["p"]["ded"],
         "TaskManager2_ProcessTasks": lambda mu: None, "DB_LoadLoadFastfilesForNewContent": lambda mu: None,
         "Cbuf_Execute": lambda mu: None, "??2@YAPEAX_K@Z": new, "??3@YAXPEAX_K@Z": lambda mu: None,
         "sub_14082AEF0": lambda mu: F["p"]["queueResult"], "sub_14034E990": lambda mu: None,
         "dwGetTwitchService": lambda mu: F["p"]["twitch"], "bdLobbyService_getMessaging": lambda mu: F["p"]["messaging"],
         "sub_1405B7C10": lambda mu: None, "sub_1407893BC": poll, "Com_TimeScaleMsec": lambda mu: (arg(mu, 0) * 3) // 2,
         "Demo_Frame": lambda mu: None, "sub_1403CE3A0": lambda mu: None,
         "sub_1403D8DA0": lambda mu: arg(mu, 1) + F["p"]["svExtra"], "sub_1403A3D80": lambda mu: None,
         "Phys_RunToTime": lambda mu: None, "sub_1405B83D0": lambda mu: None, "sub_1405B5DB0": lambda mu: None,
         "IN_Frame": lambda mu: None, "sub_140473AB0": lambda mu: None, "sub_1404797C0": lambda mu: None,
         "sub_1401AE430": lambda mu: None, "sub_140430860": lambda mu: F["p"]["value"],
         "sub_1404071C0": lambda mu: None, "sub_1400C7720": lambda mu: None, "SCR_UpdateScreen": lambda mu: None,
         "sub_140498A50": lambda mu: None, "sub_1403B6FA0": lambda mu: None, "LongSwap": queued_call}
SEEN = {"sub_1403A2FF0": "i", "Com_EventLoop": "", "Com_IsDedicatedServer": "", "TaskManager2_ProcessTasks": "i",
        "DB_LoadLoadFastfilesForNewContent": "", "Cbuf_Execute": "ii", "??2@YAPEAX_K@Z": "q", "??3@YAXPEAX_K@Z": "qq",
        "sub_14082AEF0": "qq", "sub_14034E990": "", "dwGetTwitchService": "i", "bdLobbyService_getMessaging": "q",
        "sub_1405B7C10": "iq", "sub_1407893BC": "qii", "Com_TimeScaleMsec": "i", "Demo_Frame": "ii",
        "sub_1403CE3A0": "", "sub_1403D8DA0": "xi", "sub_1403A3D80": "", "Phys_RunToTime": "i", "sub_1405B83D0": "i",
        "sub_1405B5DB0": "ii", "IN_Frame": "", "sub_140473AB0": "", "sub_1404797C0": "", "sub_1401AE430": "ii",
        "sub_140430860": "", "sub_1404071C0": "ii", "sub_1400C7720": "i", "SCR_UpdateScreen": "",
        "sub_140498A50": "i", "sub_1403B6FA0": "iis"}


def dvar(w, slot, index, dtype, value):
    """A dvar_t (x64: type +0x18, current +0x20) at DVARS + index * 0x60, or none."""
    if value is None:
        w.u64(slot, 0)
        return
    va = DVARS + index * 0x60
    w.write(va + 0x18, struct.pack("<i", dtype))
    if dtype == 8:
        w.write(DVARS + 0x1000 + index * 0x20, value.encode() + b"\0")
        w.u64(va + 0x20, DVARS + 0x1000 + index * 0x20)
    elif dtype == 2:
        w.write(va + 0x20, struct.pack("<f", value) + bytes(4))
    else:
        w.write(va + 0x20, struct.pack("<i", value) + bytes(4))
    w.u64(slot, va)


def frame_setup(p):
    def run(w):
        F.clear()
        F.update(p=p, m=w.m, clock=p["clock0"], sleeps=0, polls=0, busy=p["busy"])
        w.write(LOG, struct.pack("<I", 0))
        dvar(w, G["com_maxfps"], 0, *p["maxfps"])
        dvar(w, G["com_dedicated"], 1, 6, p["dedicated"])
        dvar(w, G["com_statmon"], 2, 1, p["statmon"])
        dvar(w, G["cl_paused"], 3, 6, p["paused"])
        dvar(w, G["sv_running"], 4, 1, p["svRunning"])
        w.u64(G["serverInfo"], SERVER_INFO)
        w.write(SERVER_INFO + 0x1C54, struct.pack("<i", p["clientFpsLimit"]))
        w.write(G["sys_timeBase"], struct.pack("<i", p["timeBase"]))
        w.write(G["initialized"], struct.pack("<i", p["initialized"]))
        w.write(G["com_frameTime"], struct.pack("<i", p["frameTime"]))
        w.write(G["com_lastFrameIndex"], struct.pack("<i", p["index"]))
        w.write(G["com_lastFrameTime"], struct.pack("<3i", *p["lft"]))
        w.write(G["com_lastFrameTime_base"], struct.pack("<3f", *p["lftb"]))
        w.write(G["primary"], struct.pack("<i", 0))
        w.write(G["demo"] + 0x88, struct.pack("<i", p["demoState"]))
        w.write(G["cls"] + 0x150, struct.pack("<ifii", 7, 0.5, 1000, 7) + bytes([p["inputUpdated"]]))
        w.write(G["store_140E61798"], struct.pack("<i", p["mode"]))
        w.write(G["store_14413FEED"], bytes([p["queue"]]))
        w.write(G["store_14413FEE4"], struct.pack("<i", 1 if p["busy"] else 0))
        w.write(0x142161684, struct.pack("<I", p["flags"]))
        w.write(G["store_14413FEE8"], struct.pack("<i", p["queueCount"]))
        for k in range(p["queueCount"]):
            w.write(QUEUE + 0x18 * k, struct.pack("<B7xQQ", k % 3 != 1, callback_va(), 0x1000 + k))
        w.u64(G["store_143ED0528"], 0x7777 if p["task"] else 0)
        w.write(G["store_143E039FF"], bytes([p["taskDone"]]))
        w.write(G["store_143ED03D4"], struct.pack("<i", p["taskSince"]))
        w.write(G["store_143ED0578"], bytes([p["twitchPending"]]))
        w.write(G["store_143E33FED"], bytes([p["twitchReady"]]))
        w.write(G["store_143C0A758"], struct.pack("<i", p["notFirst"]))
        w.write(G["store_142155E08"], struct.pack("<i", p["warnFile"]))
        w.write(G["store_1444BA284"], struct.pack("<i", p["statmonLast"]))
        w.write(G["store_140E5CB94"], struct.pack("<i", p["frameTime"] - p["sinceLast"]))
        w.write(G["store_140A5AAAB"], bytes([p["single"]]))
        w.write(G["s_gamePads"], bytes([p["padEnabled"]]) + bytes(0x4F) + struct.pack("<HH", 0x1234, 0x5678))
        w.write(TASK, bytes([0xAB]) * 0x38)
    return run


def frame_inputs(n=500, seed=0xA4010):
    r = random.Random(seed)
    out = []
    for k in range(n):
        frame_time = r.randrange(1000, 100000)
        p = {"maxfps": r.choice([(6, 0), (6, 60), (6, 125), (6, 250), (6, 333), (6, 1000), (6, 2000), (6, -5), (1, 1),
                                 (2, 85.9), (8, "144"), (None, None)]),
             "clientFpsLimit": r.choice([0, 0, 0, 125, 250, -1]), "dedicated": r.choice([0, 0, 0, 0, 1]),
             "statmon": r.choice([0, 1]), "paused": r.choice([0, 0, 1]), "svRunning": r.choice([0, 1]),
             "ded": r.choice([0, 0, 1]), "timeBase": r.randrange(1 << 20), "initialized": r.choice([1, 1, 1, 0]),
             "frameTime": frame_time, "index": r.randrange(0, 50), "lft": [r.randrange(0, 60) for _ in range(3)],
             "lftb": [r.choice([0.0, 0.016, 0.04, 0.008, 0.1]) for _ in range(3)], "demoState": r.choice([0, 0, 2]),
             "inputUpdated": r.randrange(2), "mode": r.choice([0, 0, 2, 10, 3]), "queue": r.randrange(2),
             "queueCount": r.choice([0, 1, 4]), "callMs": r.choice([0, 3, 5, 10, 11]), "task": r.randrange(2),
             "busy": r.choice([0, 0, 2]), "flags": r.choice([0, 3, 0xFFFFFFFF]), "sleepMs": r.choice([1, 1, 1, 0, 2]),
             "taskDone": r.choice([0, 0, 1]), "taskSince": r.choice([0, 999999]), "queueResult": r.choice([0, -1]),
             "twitchPending": r.randrange(2), "twitchReady": r.randrange(2), "twitch": r.choice([0, 0x7100]),
             "messaging": r.choice([0, 0x7200]), "polls": r.choice([0, 0, 2]), "svController": r.choice([0, -1]),
             "svExtra": r.choice([0, 0, 3]), "notFirst": r.randrange(2), "value": r.choice([1, 16, 17, -3]),
             "tryEnter": r.randrange(2), "single": r.randrange(2), "padEnabled": r.randrange(2),
             "warnFile": r.randrange(2), "statmonLast": r.choice([0, frame_time - 50, frame_time + 100]),
             "sinceLast": r.choice([0, 5, 300, -4]), "step": r.choice([0, 0, 1]), "yield": r.choice([0, 1]),
             "eventLoopMs": r.choice([0, 0, 1, 7, 45]), "clock0": 0}
        # the clock starts so that the frame is due in 0 to a few milliseconds, or long overdue (a hitch)
        p["clock0"] = (p["timeBase"] + frame_time + r.choice([0, 1, 2, 5, 17, 39, 40, 41, 60, 300, 7000, -3])) & 0xFFFFFFFF
        # deadlines that land on their boundary: the online task's 30 s, com_statmon's 33 ms
        now = frame_time + r.randrange(0, 45)
        p["taskSince"] = r.choice([0, 999999, now - 30000, now - 29999, now - 30001])
        p["statmonLast"] = r.choice([0, now - 33, now - 34, now - 32, now + 100])
        out.append({"args": [], "setup": frame_setup(p), "params": p})
    # boundaries random inputs rarely reach: a frame average below the frame cap, a NaN average, the online task's
    # 30 s exactly (the clock is then fully scripted: maxfps 0, one Sleep(1), nothing else moves it)
    for k in range(30):
        p = dict(out[k]["params"], maxfps=(6, r.choice([0, 60, 125])), clientFpsLimit=0, dedicated=0, step=0,
                 eventLoopMs=0, sleepMs=1, queue=0, busy=0, demoState=0)
        p["clock0"] = (p["timeBase"] + p["frameTime"]) & 0xFFFFFFFF
        if k < 10:
            p["lft"] = [r.choice([-40, -100, 2]) for _ in range(3)]
        elif k < 20:
            p["lftb"] = [float("nan"), 0.016, float("nan")] if k % 2 else [float("nan")] * 3
        else:
            p.update(maxfps=(6, 0), task=1, taskDone=0, queueResult=0,
                     taskSince=p["frameTime"] + 1 - 30000 + (k % 3 - 1))
        out.append({"args": [], "setup": frame_setup(p), "params": p})
    return out


_CALLBACK = []


def callback_va():
    """The queued calls' function: a function the frame never calls otherwise (LongSwap), faked and observed."""
    if not _CALLBACK:
        import symbols
        _CALLBACK.append(next(va for s, (va, kind) in symbols.table(STORE).items()
                              if kind == "function" and symbols.scoped_name(s) == "LongSwap"))
    return _CALLBACK[0]


def frame_cases():
    return frame_inputs()


SEEN_ALL = dict(SEEN, LongSwap="q")

CASES = [
    {"name": "Com_Frame_Try_Block_Function", "ret": "void", "builds": [STORE], "fakes": FAKES, "observe": SEEN_ALL,
     "imports": IMPORTS, "inputs": frame_cases()},
]
