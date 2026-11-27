"""client/screen_placement.cpp on the server (the Store build: tests/replay/verify_screen_placement.py)."""
import random
import struct

SERVER = ["CoDMPServer_PC"]
SCR_PLACE_VIEW, CG_HUD_SPLITSCREEN_SCALE, CLIENT_GAME_STATES = 0x0132FED8, 0x0132FF54, 0x01D77360
rng = random.Random(0x534FD0)


def f():
    return rng.randrange(-2048, 2049) * 0.0625


def placement():
    return struct.pack("<31f", *(f() for _ in range(31)))


def state(view=None, scale=None, ui_context=0):
    view, scale = view or placement(), f() if scale is None else scale

    def run(w):
        w.write(SCR_PLACE_VIEW, view)
        w.write(CG_HUD_SPLITSCREEN_SCALE, struct.pack("<f", scale))
        w.write(CLIENT_GAME_STATES, struct.pack("<6i", 0, 0, 0, ui_context, 0, 0))
    return run


USE_SCALE = [0]


def use_scale(on):
    def run(w):
        USE_SCALE[0] = on
    return run


CASES = [
    {"name": "ScrPlace_HiResGetScaleX", "ret": "float", "builds": SERVER, "inputs": [{"args": []}]},
    {"name": "ScrPlace_GetView", "ret": "uint", "builds": SERVER, "inputs": [{"args": [0], "setup": state()}]},
    {"name": "ScrPlace_ApplyRect", "ret": "void", "builds": SERVER,
     # cg_hudSplitscreenOffsetsUseScale, through Dvar_GetBool: whether the position scales too
     "fakes": {"?Dvar_GetBool@@YA_NPBUdvar_t@@@Z": lambda mu: USE_SCALE[0]},
     "inputs": [{"args": [placement(), struct.pack("<f", f()), struct.pack("<f", f()), struct.pack("<f", f()),
                          struct.pack("<f", f()), h, v], "setup": use_scale(h & 1)} for h in range(11) for v in range(11)]},
    {"name": "ScrPlace_VirtualFromReal_Height", "ret": "float", "builds": SERVER,
     "inputs": [{"args": [placement(), ("f", f())], "setup": state()} for _ in range(40)]},
]
