"""ui/ui_main.cpp: the UI context lookup and the cursor (server; the Store build inlines these into IN_MouseMove,
whose Store case covers them there).

uiInfoArray, scrPlaceFull and clientGameStates are the server's own globals; the cursor fields, isCursorVisible and
the calls to CL_ShowSystemCursor and Display_MouseMove are compared.
"""
import struct

SERVER = ["CoDMPServer_PC"]
UI = 0x078B47F8                  # uiInfoArray[0]: cursor +0x10, isCursorVisible +0x18, menuCount +0x14F4,
                                 # currentMenuType +0x21D0
SCR_PLACE = 0x0132FF58           # scrPlaceFull: scaleVirtualToFull +8
CLIENT_GAME_STATES = 0x01D77360  # uiContextIndex +0xC


def setup(context=0, lc=0, scale=(2.0, 1.5), visible=1, menus=1, menu_type=0):
    def run(w):
        w.write(CLIENT_GAME_STATES, struct.pack("<iiiiii", 0, lc, 0, context, 0, 1))
        w.write(SCR_PLACE + 8, struct.pack("<ff", *scale))
        w.write(UI + 0x10, struct.pack("<ffi", 7.0, 8.0, visible))
        w.write(UI + 0x14F4, struct.pack("<i", menus))
        w.write(UI + 0x21D0, struct.pack("<i", menu_type))
    return run


ASSERT = {"Assert_MyHandler": lambda mu: 1}
CURSOR_FAKES = {"CL_ShowSystemCursor": lambda mu: None, "Display_MouseMove": lambda mu: 0, **ASSERT}
CURSOR_SEEN = {"CL_ShowSystemCursor": "i", "Display_MouseMove": "iq", "Assert_MyHandler": "siiss"}
POINTS = [(0, 0), (1280, 720), (1281, 721), (-1, 5), (5, -1), (640, 360), (1279, 719), (2000, 100), (333, 477)]
SCALES = [(2.0, 1.5), (1.0, 1.0), (3.0, 2.25), (1.3333334, 1.5), (0.0, 1.0)]

CASES = [
    {"name": "UI_GetInfo", "ret": "int", "builds": SERVER, "fakes": ASSERT, "observe": {"Assert_MyHandler": "siiss"},
     "inputs": [{"args": [0], "setup": setup(context=c, lc=l)} for c in (0, 1, -1) for l in (0, 1)]},
    {"name": "UI_KeysBypassMenu", "ret": "bool", "builds": SERVER, "fakes": ASSERT,
     "inputs": [{"args": [0], "setup": setup(menu_type=t)} for t in (0, 8, 7, 9, -8)]},
    {"name": "UI_UpdateMouseScreenPosition", "ret": "void", "builds": SERVER, "fakes": CURSOR_FAKES,
     "observe": CURSOR_SEEN,
     "inputs": [{"args": [0, x, y], "setup": setup(scale=sc)} for x, y in POINTS for sc in SCALES]},
    {"name": "UI_MouseEvent", "ret": "void", "builds": SERVER, "fakes": CURSOR_FAKES, "observe": CURSOR_SEEN,
     "inputs": [{"args": [0, 3, 4], "setup": setup(visible=v, menus=n)} for v in (0, 1, 2) for n in (0, 1, 5, -1)]},
]
