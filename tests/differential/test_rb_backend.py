"""gfx_d3d/rb_backend.cpp: RB_SwapBuffers on the server and the Store build.

COM objects come from com.py's model (every method call is compared, with its arguments); the Win32 calls (cursor
count, message queue, window, events, critical sections) are modelled here and leave their effects in the
harness's OBJECTS region, whose writes are compared.
"""
import random
import struct

from unicorn import x86_const as x86

import symbols
from com import ComModel
from harness import OBJECTS

SERVER, STORE = "CoDMPServer_PC", "t6mp_msstore"
STATE = {}

ADDR = {
    SERVER: {"dx": 0x083DA800, "r_vsync": 0x083D1F44, "r_loadingScreen": 0x083D214C, "r_texFilterQuality": 0x083D21B8,
             "r_warmShaders": 0x083D1F60, "r_fullscreen": 0x083D1E74, "r_convergence": 0x083D20D8,
             "g_showCursor": 0x07AB770C, "g_showGameCursor": 0x078AC6F8, "s_samplerStates": 0x08503D18,
             "win32QuitEvent": 0x0780031C},
    STORE: {"dx": 0x1427F1D80, "r_vsync": 0x1427E70C8, "r_loadingScreen": 0x1427E6FB8,
            "r_texFilterQuality": 0x1427E71A8, "r_warmShaders": 0x1427E7228, "r_fullscreen": 0x1427E7230,
            "r_convergence": 0x1427E71C8, "g_showCursor": 0x1421E2460, "g_showGameCursor": 0x142121314,
            "s_samplerStates": 0x142C48470, "win32QuitEvent": 0x14406FBD8, "capture": 0x1427F1B60,
            "capture_view": 0x142C3F7D8, "capture_resolve": 0x143063C70, "capture_copy": 0x143063C78,
            "capture_event": 0x143063C80, "flag_a": 0x1421E2467, "flag_b": 0x14421181E},
}
# DxGlobals fields (x86, x64 Store)
DX = {"context": (0x8, 0x10), "nvStereoActivated": (0x35, 0x51), "nvStereoHandle": (0x38, 0x58),
      "needsShaderWarming": (0x448, 0x488), "resolutionCallback": (0x37AF4, 0x3C7B0),
      "targetWindowIndex": (0x37B08, 0x3C7D0), "windowCount": (0x37B0C, 0x3C7D4), "windows": (0x37B10, 0x3C7D8),
      "swapChain": (0x37B24, 0x3C7F8)}

# dx.resolutionCallback is only ever set at run time: the test points it at a function both builds identify and the
# frame never reaches (LongSwap), faked, so the call and its argument are observed
CALLBACK = "LongSwap"


def function_va(build, name):
    return next(va for s, (va, k) in symbols.table(build).items() if symbols.scoped_name(s) == name and k == "function")


LOG = OBJECTS + 0x1F0000   # ShowCursor count, then a log of Win32 effects (count, entries)
SHOW_COUNT, LOG_N, LOG_ENTRIES = LOG, LOG + 4, LOG + 8
DVARS = OBJECTS + 0x1E0000
WM_QUIT = 0x12

SIGS = {("IDXGISwapChain", 8 * 8): "ii", ("IDXGISwapChain", 11 * 8): "pp", ("ID3D11View", 7 * 8): "p",
        ("ID3D11Resource", 2 * 8): "", ("ID3D11State", 2 * 8): "",
        ("ID3D11DeviceContext", 47 * 8): "pp", ("ID3D11DeviceContext", 57 * 8): "pipii"}


def x64():
    return STATE["build"] == STORE


def ptr():
    return "<Q" if x64() else "<I"


def arg(mu, i):
    if x64():
        return mu.reg_read((x86.UC_X86_REG_RCX, x86.UC_X86_REG_RDX, x86.UC_X86_REG_R8, x86.UC_X86_REG_R9)[i])
    return struct.unpack("<I", bytes(mu.mem_read(mu.reg_read(x86.UC_X86_REG_ESP) + 4 + 4 * i, 4)))[0]


def result(mu, value):
    mu.reg_write(x86.UC_X86_REG_RAX if x64() else x86.UC_X86_REG_EAX, value & 0xFFFFFFFF)


def log(m, *values):
    """Appends a Win32 effect to the compared log."""
    n = struct.unpack("<I", bytes(m.mu.mem_read(LOG_N, 4)))[0]
    m.program_write(LOG_ENTRIES + n * 16, struct.pack("<QQ", *(list(values) + [0, 0])[:2]))
    m.program_write(LOG_N, struct.pack("<I", n + 1))


# -- Win32, modelled
def ShowCursor(m, mu):
    count = struct.unpack("<i", bytes(mu.mem_read(SHOW_COUNT, 4)))[0] + (1 if arg(mu, 0) & 0xFFFFFFFF else -1)
    m.program_write(SHOW_COUNT, struct.pack("<i", count))
    result(mu, count)


def write_msg(mu, va, message):
    msg, time = message
    if x64():
        mu.mem_write(va, struct.pack("<QI4xQQI", 0x5150, msg, 7, 9, time))
    else:
        mu.mem_write(va, struct.pack("<IIIII", 0x5150, msg, 7, 9, time))


def PeekMessageA(m, mu):
    queue = STATE["messages"]
    remove = (struct.unpack("<I", bytes(mu.mem_read(mu.reg_read(x86.UC_X86_REG_RSP) + 0x28, 4)))[0] if x64()
              else struct.unpack("<I", bytes(mu.mem_read(mu.reg_read(x86.UC_X86_REG_ESP) + 20, 4)))[0]) & 1
    had = bool(queue)  # PM_REMOVE takes the message off the queue; PM_NOREMOVE leaves it for GetMessage
    if had:
        write_msg(mu, arg(mu, 0), queue.pop(0) if remove else queue[0])
    result(mu, 1 if had else 0)


def GetMessageA(m, mu):
    if not STATE["messages"]:  # would block until a message comes: log it, and hand back a WM_QUIT
        log(m, 0xB10C, 0)
        STATE["messages"].append((WM_QUIT, 0))
    message = STATE["messages"].pop(0)
    write_msg(mu, arg(mu, 0), message)
    result(mu, 0 if message[0] == WM_QUIT else 1)


def TranslateMessage(m, mu):
    log(m, 0x7E, struct.unpack("<I", bytes(mu.mem_read(arg(mu, 0) + (8 if x64() else 4), 4)))[0])
    result(mu, 0)


def DispatchMessageA(m, mu):
    log(m, 0xD1, struct.unpack("<I", bytes(mu.mem_read(arg(mu, 0) + (8 if x64() else 4), 4)))[0])
    result(mu, 0)


def ShowWindow(m, mu):
    log(m, 0x5A, arg(mu, 0), )
    log(m, 0x5B, arg(mu, 1) & 0xFFFFFFFF)
    result(mu, 1)


def SetEvent(m, mu):
    log(m, 0xE7, arg(mu, 0))
    result(mu, 1)


def EnterCriticalSection(m, mu):
    log(m, 0xCE, arg(mu, 0))


def LeaveCriticalSection(m, mu):
    log(m, 0xCF, arg(mu, 0))


def D3DPERF_EndEvent(m, mu):
    log(m, 0xEE, 0)
    result(mu, 0)


IMPORTS = {"user32.dll!ShowCursor": (ShowCursor, 1), "user32.dll!PeekMessageA": (PeekMessageA, 5),
           "user32.dll!GetMessageA": (GetMessageA, 4), "user32.dll!TranslateMessage": (TranslateMessage, 1),
           "user32.dll!DispatchMessageA": (DispatchMessageA, 1), "user32.dll!ShowWindow": (ShowWindow, 2),
           "kernel32.dll!SetEvent": (SetEvent, 1), "kernel32.dll!EnterCriticalSection": (EnterCriticalSection, 1),
           "kernel32.dll!LeaveCriticalSection": (LeaveCriticalSection, 1),
           "d3d9.dll!D3DPERF_EndEvent": (D3DPERF_EndEvent, 0)}


def stereo_is_activated(mu):
    mu.mem_write(arg(mu, 1), bytes([STATE["stereo_on"]]))
    return STATE["stereo_status"]


FAKES = {"Dvar_ValuesEqual": lambda mu: STATE["equal"], "Dvar_SetVariant": lambda mu: None,
         "Material_WarmAllShaders": lambda mu: None, "NvAPI_Stereo_IsActivated": stereo_is_activated,
         "NvAPI_Stereo_SetConvergence": lambda mu: 0}
FAKES[CALLBACK] = lambda mu: None
SEEN = {CALLBACK: "b", "Dvar_ValuesEqual": "i", "Dvar_SetVariant": "q", "Material_WarmAllShaders": "",
        "NvAPI_Stereo_IsActivated": "q", "NvAPI_Stereo_SetConvergence": "qf"}
SERVER_FAKES = dict(FAKES, PIXBeginNamedEvent=lambda mu: None, Sys_IsRenderThread=lambda mu: STATE["render_thread"],
                    TRACK_bg_weapons=lambda mu: None, Assert_MyHandler=lambda mu: 1)
SERVER_SEEN = dict(SEEN, PIXBeginNamedEvent="is", TRACK_bg_weapons="", Assert_MyHandler="siiss")
STORE_FAKES = dict(FAKES, store_1404B15B0=lambda mu: None, store_1407362F0=lambda mu: None,
                   store_140487390=lambda mu: None)
STORE_SEEN = dict(SEEN, store_1404B15B0="", store_1407362F0="", store_140487390="")


def dvar(w, slot, index, dtype, value, latched=None):
    """A dvar_t at DVARS + index * 0x80 (x86: type +0x10, current +0x18, latched +0x28; x64: +0x18, +0x20, +0x30)."""
    va = DVARS + index * 0x80
    t, cur, lat = (0x18, 0x20, 0x30) if x64() else (0x10, 0x18, 0x28)
    w.write(va + t, struct.pack("<i", dtype))
    enc = (lambda v: struct.pack("<f", v) + bytes(12)) if dtype == 2 else (lambda v: struct.pack("<i", v) + bytes(12))
    w.write(va + cur, enc(value))
    w.write(va + lat, enc(value if latched is None else latched))
    w.write(slot, struct.pack(ptr(), va))


def make_setup(build, p):
    def run(w):
        STATE.clear()
        STATE.update(build=build, messages=list(p["messages"]), equal=p["equal"], stereo_on=p["stereo_on"],
                     stereo_status=p["stereo_status"], render_thread=p["render_thread"])
        a = ADDR[build]
        i = 1 if build == STORE else 0
        model = ComModel(SIGS)
        model.install(w.m)
        dx = a["dx"]
        swap = [model.new("IDXGISwapChain") for _ in range(2)]
        context = model.new("ID3D11DeviceContext")
        fullscreen_state = p["fullscreen_state"]

        def get_fullscreen(call):
            call.write(call.arg(0), struct.pack("<i", fullscreen_state))
            return 0
        model.results[("IDXGISwapChain", 11 * 8)] = get_fullscreen
        model.results[("ID3D11View", 7 * 8)] = lambda call: call.out_object(0, "ID3D11Resource") and 0
        w.write(dx + DX["context"][i], struct.pack(ptr(), context))
        w.write(dx + DX["targetWindowIndex"][i], struct.pack("<ii", p["target"], 2))
        size = 0x20 if build == STORE else 0x14
        for k in range(2):
            w.write(dx + DX["windows"][i] + k * size, struct.pack(ptr(), 0x5150 + k) + struct.pack(ptr(), swap[k]))
        w.write(dx + DX["swapChain"][i], struct.pack(ptr(), swap[0]))
        w.write(dx + DX["needsShaderWarming"][i], bytes([p["warming"]]))
        w.write(dx + DX["resolutionCallback"][i],
                struct.pack(ptr(), function_va(build, CALLBACK) if p["callback"] else 0))
        w.write(dx + DX["nvStereoHandle"][i], struct.pack(ptr(), 0x57E0 if p["stereo"] else 0))
        w.write(dx + DX["nvStereoActivated"][i], bytes([p["stereo_was"]]))
        dvar(w, a["r_vsync"], 0, 1, p["vsync"])
        dvar(w, a["r_loadingScreen"], 1, 1, p["loading"])
        dvar(w, a["r_texFilterQuality"], 2, 6, 1, p["latched"])
        dvar(w, a["r_warmShaders"], 3, 1, p["warm"])
        dvar(w, a["r_fullscreen"], 4, 6, p["fullscreen"])
        dvar(w, a["r_convergence"], 5, 2, 6.5)
        w.write(a["g_showCursor"], struct.pack("<i", p["show"]))
        w.write(a["g_showGameCursor"], struct.pack("<i", p["show_game"]))
        w.write(SHOW_COUNT, struct.pack("<iI", p["count"], 0))
        samplers = a["s_samplerStates"]
        for k in range(256):
            w.write(samplers + k * (8 if build == STORE else 4),
                    struct.pack(ptr(), model.new("ID3D11State") if k % 37 == 3 else 0))
        w.write(a["win32QuitEvent"], struct.pack(ptr(), 0xE0E0))
        if build == STORE:
            w.write(a["capture"], struct.pack("<i", p["capture"]))
            w.write(a["capture_view"], struct.pack(ptr(), model.new("ID3D11View")))
            w.write(a["capture_resolve"], struct.pack(ptr(), model.new("ID3D11Resource")))
            w.write(a["capture_copy"], struct.pack(ptr(), model.new("ID3D11Resource")))
            w.write(a["capture_event"], struct.pack(ptr(), 0xCAFE))
            w.write(a["flag_a"], bytes([p["flag_a"]]))
            w.write(a["flag_b"], bytes([p["flag_b"]]))
    return run


def inputs(build, n=300, seed=7158):
    r = random.Random(seed)
    out = []
    for k in range(n):
        p = {"target": r.choice([0, 0, 1]), "vsync": r.randrange(2), "loading": r.randrange(2),
             "equal": r.choice([1, 1, 0]), "latched": r.choice([1, 2]), "callback": r.randrange(2),
             "warming": r.randrange(2), "warm": r.randrange(2), "show": r.choice([0, 0, 1, 2]),
             "show_game": r.choice([0, 0, 1]), "count": r.choice([0, -1, 3, -4, 1]),
             "messages": [(r.choice([0x100, 0x200, 0x0F, 0x113]), r.randrange(1 << 31))
                          for _ in range(r.choice([0, 0, 1, 3]))] + ([(WM_QUIT, 5)] if r.random() < 0.1 else []),
             "fullscreen": r.choice([0, 1, 1, 2]), "fullscreen_state": r.randrange(2), "stereo": r.randrange(2),
             "stereo_on": r.randrange(2), "stereo_status": r.choice([0, 0, -1]), "stereo_was": r.randrange(2),
             "render_thread": r.randrange(2), "capture": r.choice([0, 0, 0, 1]), "flag_a": r.randrange(2),
             "flag_b": r.randrange(2)}
        if k == 0:
            p.update(target=5)  # out of range: the server's assert
        out.append({"args": [], "setup": make_setup(build, p), "params": p})
    return out


CASES = [
    {"name": "RB_SwapBuffers", "ret": "void", "builds": [SERVER, STORE], "fakes": SERVER_FAKES, "observe": SERVER_SEEN,
     "imports": IMPORTS, "inputs": inputs(SERVER)[1:],
     "by_build": {STORE: {"fakes": STORE_FAKES, "observe": STORE_SEEN, "imports": IMPORTS,
                          "inputs": inputs(STORE)[1:]}}},
]


# ---- RB_RenderThread (Store 140736CD0) ------------------------------------------------------------------------------
# The thread never returns: each input scripts what its waits see, and the run ends (Machine.end_call) when the script
# runs out, at the noreturn error path, or after a fixed number of screen updates. Events, waits, Sleep, the message
# pump and critical sections are logged to the compared region; the frame's buffers are COM objects.
from harness import TLS_BLOCK  # noqa: E402

RT = {}
RT_DATA = 0x34000000                    # a back-end data block (its fields sit 0x122C00 and 0x4BE9C0 in)
RT_VALUES, RT_JMPBUF = OBJECTS + 0x10000, OBJECTS + 0x10100
RT_UPLOAD, RT_SOURCE, RT_MAPPED = OBJECTS + 0x10400, OBJECTS + 0x10500, OBJECTS + 0x11000
EV = {"rendererRunning": 0x1406FC68, "renderPaused": 0x14070050, "backend1": 0x1406FC58, "e8": 0x14070008,
      "e48": 0x14070048, "quit": 0xE0E0}
RT_ADDR = {"smpData": 0x14406FAF0, "backendEvent": 0x14406FC50, "rendererRunningEvent": 0x14406FC68,
           "renderPausedEvent": 0x144070050, "e8": 0x144070008, "e48": 0x144070048,
           "backEndData": 0x14449ED18, "busy": 0x142155DFC, "r_glob": 0x1427E7330, "vidConfig": 0x14282E5C0,
           "win32QuitEvent": 0x14406FBD8, "dx": 0x1427F1D80}


def rt_wait(m, mu):
    handle, timeout = arg(mu, 0), arg(mu, 1) & 0xFFFFFFFF
    log(m, 0x3A17, handle)
    script = RT["waits"]
    if not script:
        m.end_call()
        return
    result(mu, script.pop(0) if timeout == 0 else 0)


def rt_event(tag):
    def run(m, mu):
        log(m, tag, arg(mu, 0))
        result(mu, 1)
    return run


def rt_sleep(m, mu):
    log(m, 0x51EE, arg(mu, 0) & 0xFFFFFFFF)


RT_IMPORTS = dict(IMPORTS, **{"kernel32.dll!WaitForSingleObject": (rt_wait, 2),
                              "kernel32.dll!ResetEvent": (rt_event(0xE5), 1), "kernel32.dll!SetEvent": (rt_event(0xE7), 1),
                              "kernel32.dll!Sleep": (rt_sleep, 1)})


def rt_setjmp(mu):
    return RT["setjmp"].pop(0) if RT["setjmp"] else 0


def rt_screen_update(mu):
    RT["updates"] += 1
    if RT["updates"] >= RT["p"]["updates"]:
        RT["m"].program_write(RT_ADDR["r_glob"] + 0x10, struct.pack("<i", 0))


def rt_wnd_parms(mu):
    p = RT["p"]
    mu.mem_write(arg(mu, 0), struct.pack("<QiB3xiiiiii", 0x7777, 60, p["fs"], 0, 0, 1280, 720, *p["size"]))  # 0x28


def rt_fatal(mu):
    RT["m"].end_call()


RT_FAKES = {"_setjmp": rt_setjmp, "RB_UploadDynamicBuffers": lambda mu: None, "RB_RenderThreadFrame": lambda mu: None,
            "sub_140721570": lambda mu: None, "sub_140721860": lambda mu: None, "sub_140753660": lambda mu: None,
            "sub_140752AA0": lambda mu: None, "sub_1407370D0": lambda mu: None, "SCR_UpdateScreen": rt_screen_update,
            "R_SetWndParms": rt_wnd_parms, "R_StoreWindowSettings": lambda mu: None, "sub_14039AE90": rt_fatal,
            CALLBACK: lambda mu: None}
RT_SEEN = {"_setjmp": "q", "RB_UploadDynamicBuffers": "q", "RB_RenderThreadFrame": "", "sub_140721570": "",
           "sub_140721860": "", "sub_140753660": "", "sub_140752AA0": "", "sub_1407370D0": "", "SCR_UpdateScreen": "",
           "R_SetWndParms": "x", "R_StoreWindowSettings": "x", "sub_14039AE90": "", CALLBACK: "b"}
RT_SIGS = {("ID3D11DeviceContext", 14 * 8): "piiip", ("ID3D11DeviceContext", 15 * 8): "pi",
           ("IDXGISwapChain", 13 * 8): "iiiii", ("ID3D11Resource", 2 * 8): ""}


def rt_setup(p):
    def run(w):
        RT.clear()
        RT.update(m=w.m, p=p, waits=list(p["waits"]), setjmp=list(p["setjmp"]), updates=0)
        STATE.clear()
        STATE.update(build=STORE, messages=list(p["messages"]))
        try:
            w.m.mu.mem_map(RT_DATA, 0x600000)
        except Exception:
            pass
        model = ComModel(RT_SIGS)
        model.install(w.m)

        def map_(call):
            # the busy flag stays up for p["busy"] Maps: the original retries while it is set
            busy = struct.unpack("<i", bytes(call.mu.mem_read(RT_ADDR["busy"], 4)))[0]
            call.write(RT_ADDR["busy"], struct.pack("<i", max(busy - 1, 0)))
            call.write(call.arg(4), struct.pack("<QII", RT_MAPPED, 0, 0))
            return 0
        model.results[("ID3D11DeviceContext", 14 * 8)] = map_
        a, dx = RT_ADDR, RT_ADDR["dx"]
        # the thread's value table: [2] is its jump buffer
        w.write(TLS_BLOCK + 0xB0, struct.pack("<Q", RT_VALUES))
        w.write(RT_VALUES, struct.pack("<QQQ", 0, 0, RT_JMPBUF))
        for name, event in (("rendererRunningEvent", "rendererRunning"), ("renderPausedEvent", "renderPaused"),
                            ("e8", "e8"), ("e48", "e48")):
            w.write(a[name], struct.pack("<Q", EV[event]))
        w.write(a["backendEvent"], struct.pack("<QQ", 0x1406FC50, EV["backend1"]))
        w.write(a["win32QuitEvent"], struct.pack("<Q", EV["quit"]))
        w.write(a["smpData"], struct.pack("<Q", RT_DATA if p["frame"] else 0))
        w.write(a["backEndData"], struct.pack("<Q", p["backEndData"]))
        w.write(a["busy"], struct.pack("<i", p["busy"]))
        w.write(a["r_glob"], struct.pack("<B7xBxxxiii", 1, p["renderingRemote"], 0, p["nesting"], 0))
        w.write(a["vidConfig"], struct.pack("<IIII", 1280, 720, *p["vid"]))
        # the frame: its dynamic vertex data and the three buffer sets
        context = model.new("ID3D11DeviceContext")
        w.write(dx + 0x10, struct.pack("<Q", context))
        w.write(RT_DATA + 0x4BE9C0, struct.pack("<Q", RT_UPLOAD))
        w.write(RT_DATA + 0x122C00, struct.pack("<Q", OBJECTS + 0x12000))
        w.write(RT_UPLOAD, struct.pack("<iiQQ", p["used"], p["capacity"],
                                       model.new("ID3D11Resource") if p["buffer"] else 0, RT_SOURCE))
        w.write(RT_SOURCE, bytes(range(256)))
        w.write(RT_MAPPED, bytes(0x100))
        # resize state
        w.write(dx + 0x486, bytes([p["resize"]]))
        w.write(dx + 0x3C7D8, struct.pack("<Q", 0x5150 if p["hwnd"] else 0))
        w.write(dx + 0x3C7F8, struct.pack("<QQ", model.new("IDXGISwapChain") if p["swap"] else 0,
                                          model.new("ID3D11Resource") if p["backBuffer"] else 0))
        w.write(dx + 0x3C7B0, struct.pack("<Q", function_va(STORE, CALLBACK) if p["callback"] else 0))
        w.write(SHOW_COUNT, struct.pack("<iI", 0, 0))
    return run


def rt_inputs(n=250, seed=0x6CD0):
    r = random.Random(seed)
    out = []
    for k in range(n):
        timeouts = [r.choice([0, 0x102, 0x102, 0x102]) for _ in range(r.randrange(1, 12))]
        p = {"waits": timeouts, "setjmp": r.choice([[], [], [], [1], [1, 1], [0]]), "frame": r.random() < 0.7,
             "backEndData": r.choice([0, 0, RT_DATA]), "renderingRemote": r.randrange(2),
             "nesting": r.choice([0, 0, 0, 1]), "updates": r.choice([1, 2, 3]), "used": r.choice([0, 16, 200, 256]),
             "capacity": r.choice([0, 64, 256]), "buffer": r.random() < 0.8, "resize": r.randrange(2),
             "hwnd": r.random() < 0.8, "swap": r.random() < 0.8, "backBuffer": r.randrange(2),
             "callback": r.randrange(2), "fs": r.randrange(2), "busy": r.choice([0, 0, 1, 3]), "vid": r.choice([(1920, 1080), (2560, 1440)]),
             "size": r.choice([(1920, 1080), (2560, 1440), (1920, 1200)]),
             "messages": [(r.choice([0x100, 0x0F]), r.randrange(1 << 31)) for _ in range(r.choice([0, 0, 2]))]
             + ([(WM_QUIT, 5)] if r.random() < 0.1 else [])}
        out.append({"args": [0], "setup": rt_setup(p), "params": p})
    return out


CASES.append({"name": "RB_RenderThread", "ret": "void", "builds": [STORE], "fakes": RT_FAKES, "observe": RT_SEEN,
              "imports": RT_IMPORTS, "inputs": rt_inputs()})
