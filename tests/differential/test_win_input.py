"""win32/win_input.cpp.

The window, the cursor and ShowCursor's count are modelled in the harness's OBJECTS region, whose writes are
compared: both runs start from the same window and cursor and must leave the same.
"""
import random
import struct

from unicorn import x86_const as x86

from harness import OBJECTS

rng = random.Random(2013)
STATE = {}
SERVER = ["CoDMPServer_PC"]

G_WV = 0x07ACD89C            # WinVars_t: hWnd +8, recenterMouse +0x18
S_WMV = 0x07AB7718           # WinMouseVars_t: oldPos +4, mouseActive +0xC, mouseInitialized +0xD
WINDOW_CENTER_X, WINDOW_CENTER_Y = 0x07AB7710, 0x07AB7704
IN_APPACTIVE, G_SHOW_CURSOR = 0x07AB7714, 0x07AB770C
DVAR_PTRS = {0x07AB76FC: 0x7201, 0x083D1E74: 0x7202, 0x01E2917C: 0x7203}  # in_mouse, r_fullscreen, ClickToContinue
IN_MOUSE, R_FULLSCREEN, CLICK = 0x7201, 0x7202, 0x7203
HWND = 0x5150

MODEL = OBJECTS + 0x1F0000   # cursor x, y; window rect l, t, r, b; foreground; ShowCursor count; posted clicks
CURSOR, RECT, FOREGROUND, SHOW_COUNT, POSTED = MODEL, MODEL + 8, MODEL + 0x18, MODEL + 0x1C, MODEL + 0x20
CLIENT_ORIGIN = (8, 31)      # the client area's offset in the window (border and title bar)


def u32(mu, va):
    return struct.unpack("<I", bytes(mu.mem_read(va, 4)))[0]


def s32(mu, va):
    return struct.unpack("<i", bytes(mu.mem_read(va, 4)))[0]


def arg(mu, i):
    return u32(mu, mu.reg_read(x86.UC_X86_REG_ESP) + 4 + 4 * i)


def sarg(mu, i):
    return struct.unpack("<i", struct.pack("<I", arg(mu, i)))[0]


def result(mu, value):
    mu.reg_write(x86.UC_X86_REG_EAX, value & 0xFFFFFFFF)


# -- user32, modelled
def GetForegroundWindow(m, mu):
    result(mu, u32(mu, FOREGROUND))


def GetCursorPos(m, mu):
    mu.mem_write(arg(mu, 0), bytes(mu.mem_read(CURSOR, 8)))  # into the caller's local
    result(mu, 1)


def SetCursorPos(m, mu):
    m.program_write(CURSOR, struct.pack("<ii", sarg(mu, 0), sarg(mu, 1)))
    result(mu, 1)


def GetWindowRect(m, mu):
    mu.mem_write(arg(mu, 1), bytes(mu.mem_read(RECT, 16)))
    result(mu, 1)


def window_origin(mu):
    left, top = struct.unpack("<ii", bytes(mu.mem_read(RECT, 8)))
    return left + CLIENT_ORIGIN[0], top + CLIENT_ORIGIN[1]


def ScreenToClient(m, mu):
    x, y = struct.unpack("<ii", bytes(mu.mem_read(arg(mu, 1), 8)))
    ox, oy = window_origin(mu)
    mu.mem_write(arg(mu, 1), struct.pack("<ii", x - ox, y - oy))
    result(mu, 1)


def ClientToScreen(m, mu):
    x, y = struct.unpack("<ii", bytes(mu.mem_read(arg(mu, 1), 8)))
    ox, oy = window_origin(mu)
    mu.mem_write(arg(mu, 1), struct.pack("<ii", x + ox, y + oy))
    result(mu, 1)


def ShowCursor(m, mu):
    count = s32(mu, SHOW_COUNT) + (1 if arg(mu, 0) else -1)
    m.program_write(SHOW_COUNT, struct.pack("<i", count))
    result(mu, count)


def PostMessageA(m, mu):
    if arg(mu, 0) == HWND and arg(mu, 1) == 0x201 and arg(mu, 2) == 1 and arg(mu, 3) == 0:
        m.program_write(POSTED, struct.pack("<I", u32(mu, POSTED) + 1))
    result(mu, 1)


IMPORTS = {f"user32.dll!{f.__name__}": (f, n) for f, n in [
    (GetForegroundWindow, 0), (GetCursorPos, 1), (SetCursorPos, 2), (GetWindowRect, 2), (ScreenToClient, 2),
    (ClientToScreen, 2), (ShowCursor, 1), (PostMessageA, 4)]}


def setup(cursor=(500, 400), rect=(100, 50, 1380, 770), foreground=HWND, show_count=0, old=(500, 400), active=1,
          initialized=1, appactive=1, dvars=None, **state):
    def run(w):
        STATE.clear()
        STATE.update(state)
        STATE["dvars"] = dvars or {}
        w.write(CURSOR, struct.pack("<ii", *cursor))
        w.write(RECT, struct.pack("<iiii", *rect))
        w.u32(FOREGROUND, foreground)
        w.write(SHOW_COUNT, struct.pack("<i", show_count))
        w.u32(G_WV + 8, HWND)
        w.write(S_WMV + 4, struct.pack("<iiBB", old[0], old[1], active, initialized))
        w.u32(IN_APPACTIVE, appactive)
        for va, handle in DVAR_PTRS.items():
            w.u32(va, handle)
    return run


def gpad_float(name):
    return lambda mu: float(STATE.get("gpad", {}).get((name, arg(mu, 1)), 0.0))


def gpad_bool(name):
    return lambda mu: int(arg(mu, 1) in STATE.get(name, ()))


FAKES = {
    "?Dvar_GetBool@@YA_NPBUdvar_t@@@Z": lambda mu: int(bool(STATE["dvars"].get(arg(mu, 0), 0))),
    "?Dvar_GetInt@@YAHPBUdvar_t@@@Z": lambda mu: STATE["dvars"].get(arg(mu, 0), 0),
    "CL_MouseEvent": lambda mu: STATE.get("recenter", 1),
    "Com_LocalClients_GetPrimary": lambda mu: 0,
    "Com_LocalClient_LastInput_Set": lambda mu: None,
    "Com_ControllerIndex_GetLocalClientNum": lambda mu: 0,
    "Sys_Milliseconds": lambda mu: STATE.get("time", 123456),
    "GPad_UpdateAll": lambda mu: None,
    "GPad_IsActive": lambda mu: STATE.get("gpad_active", 1),
    "GPad_GetStick": gpad_float("stick"),
    "GPad_GetButton": gpad_float("button"),
    "GPad_IsButtonPressed": gpad_bool("pressed"),
    "GPad_ButtonRequiresUpdates": gpad_bool("held"),
    "GPad_IsButtonReleased": gpad_bool("released"),
    "CL_GamepadEvent": lambda mu: None,
    "CL_GamepadButtonEventForPort": lambda mu: None,
    "Assert_MyHandler": lambda mu: 1,
}
SEEN = {
    "?Dvar_GetBool@@YA_NPBUdvar_t@@@Z": "q", "?Dvar_GetInt@@YAHPBUdvar_t@@@Z": "q", "CL_MouseEvent": "iiiii",
    "Com_LocalClient_LastInput_Set": "ii", "GPad_GetStick": "ii", "GPad_GetButton": "ii",
    "CL_GamepadEvent": "iiii", "CL_GamepadButtonEventForPort": "iiiii", "Assert_MyHandler": "siiss",
    "GPad_IsButtonPressed": "ii", "GPad_ButtonRequiresUpdates": "ii", "GPad_IsButtonReleased": "ii",
    "IN_GamepadsMove": "", "IN_MouseMove": "", "IN_ActivateMouse": "i", "IN_DeactivateWin32Mouse": "",
}

PAD_BUTTONS = [0x10002000, 0x10001000, 0x10004000, 0x10008000, 0x20000000, 0x20000001, 0x10000100, 0x10000200,
               0x10000010, 0x10000020, 0x10000040, 0x10000080, 0x10000001, 0x10000002, 0x10000004, 0x10000008]
STICKS = [0x40000000, 0x40000001, 0x40000002, 0x40000003]


def mouse_inputs(n=150):
    out = []
    for _ in range(n):
        rect = rng.choice([(100, 50, 1380, 770), (0, 0, 1920, 1080), (-1920, 0, 0, 1080)])
        cursor = rng.choice([(rect[0] + 640, rect[1] + 360), (rect[0] - 5, rect[1] + 3), (rect[2], rect[3]),
                             (rect[2] + 40, rect[1] - 1), (rect[0] + rng.randrange(-50, 1400), rect[1] + rng.randrange(-50, 800))])
        old = rng.choice([cursor, (rect[0] + 640, rect[1] + 360), (cursor[0] - 3, cursor[1] + 7)])
        out.append({"args": [], "setup": setup(cursor=cursor, rect=rect, old=old,
                                               foreground=rng.choice([HWND, HWND, HWND, 0x9999]),
                                               initialized=rng.choice([1, 1, 1, 0]),
                                               dvars={R_FULLSCREEN: rng.choice([0, 1, 2])},
                                               recenter=rng.randrange(2))})
    return out


def gamepad_inputs(n=60):
    out = []
    for _ in range(n):
        # the last four: a float product would round across an integer (the original truncates it unrounded)
        gpad = {("stick", s): rng.choice([0.0, 1.0, -1.0, 0.5, rng.uniform(-1, 1), 1e-5, -0.37358662486076355,
                                          0.3145647346973419, 0.41866177320480347, -0.1759517788887024])
                for s in STICKS}
        gpad.update({("button", b): rng.choice([0.0, 1.0, rng.uniform(0, 1)]) for b in (0x20000000, 0x20000001)})
        picks = lambda: set(rng.sample(PAD_BUTTONS, rng.randrange(0, 4)))
        out.append({"args": [], "setup": setup(gpad=gpad, pressed=picks(), held=picks(), released=picks(),
                                               gpad_active=rng.choice([1, 1, 0]))})
    out.append({"args": [], "setup": setup(gpad={}, pressed=set(), held=set(), released=set())})
    return out


def frame_inputs(n=60):
    out = []
    for _ in range(n):
        out.append({"args": [], "setup": setup(initialized=rng.choice([1, 1, 0]), appactive=rng.choice([1, 0]),
                                               active=rng.randrange(2), foreground=rng.choice([HWND, 0x9999]),
                                               show_count=rng.choice([-1, 0, -3]),
                                               dvars={IN_MOUSE: rng.choice([1, 1, 0]), CLICK: rng.choice([0, 0, 1])})})
    return out


# IN_Frame's callees are reconstructed too: observing them checks it calls them, and faking them keeps its case to
# IN_Frame itself
FRAME_FAKES = dict(FAKES, IN_GamepadsMove=lambda mu: None, IN_MouseMove=lambda mu: None,
                   IN_ActivateMouse=lambda mu: None)

CASES = [
    {"name": "IN_MouseMove", "ret": "void", "builds": SERVER, "imports": IMPORTS, "fakes": FAKES, "observe": SEEN,
     "inputs": mouse_inputs()},
    {"name": "IN_ClampMouseMove", "ret": "void", "builds": SERVER, "imports": IMPORTS, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [struct.pack("<ii", x, y)], "setup": setup(rect=(100, 50, 1380, 770))}
                for x in (99, 100, 101, 1379, 1380, 5000) for y in (49, 50, 400, 769, 770, -9)]},
    {"name": "IN_RecenterMouse", "ret": "void", "builds": SERVER, "imports": IMPORTS,
     "inputs": [{"args": [], "setup": setup(rect=r)} for r in ((100, 50, 1380, 770), (-1921, -1, 0, 1080), (0, 0, 1, 1))]},
    {"name": "IN_SetCursorPos", "ret": "void", "builds": SERVER, "imports": IMPORTS,
     "inputs": [{"args": [x, y], "setup": setup()} for x, y in ((0, 0), (640, 360), (-10, 5000))]},
    {"name": "IN_ShowSystemCursor", "ret": "void", "builds": SERVER, "imports": IMPORTS,
     "inputs": [{"args": [s], "setup": setup(show_count=c)} for s in (0, 1, 7) for c in (-4, -1, 0, 1, 3)]},
    {"name": "IN_DeactivateWin32Mouse", "ret": "void", "builds": SERVER, "imports": IMPORTS,
     "inputs": [{"args": [], "setup": setup(show_count=c)} for c in (-4, -1, 0, 1, 3)]},
    {"name": "IN_ActivateMouse", "ret": "void", "builds": SERVER, "imports": IMPORTS, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [f], "setup": setup(initialized=i, active=a, foreground=fg, dvars={IN_MOUSE: m})}
                for f in (0, 1) for i in (0, 1) for a in (0, 1) for fg in (HWND, 0x9999) for m in (0, 1)]},
    {"name": "IN_Shutdown", "ret": "void", "builds": SERVER, "imports": IMPORTS,
     "inputs": [{"args": [], "setup": setup(initialized=i, active=a, show_count=-1)} for i in (0, 1) for a in (0, 1)]},
    {"name": "IN_GamepadsMove", "ret": "void", "builds": SERVER, "imports": IMPORTS, "fakes": FAKES, "observe": SEEN,
     "inputs": gamepad_inputs()},
    {"name": "IN_Frame", "ret": "void", "builds": SERVER, "imports": IMPORTS, "fakes": FRAME_FAKES, "observe": SEEN,
     "inputs": frame_inputs()},
]
