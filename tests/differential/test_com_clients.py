"""qcommon/com_clients.cpp: the local clients' controller and last-input records (server; the Store build inlines
them, and CL_MouseMove's Store test covers them there).

clientGameStates and LastInput_Set's resetTime are the server's own globals; Sys_Milliseconds is faked so the 200 ms
debounce can be stepped through.
"""
import struct

STATE = {}
SERVER = ["CoDMPServer_PC"]
CLIENT_GAME_STATES = 0x01D77360  # ClientGameState[1]: localClientNum +4, controllerIndex +8, lastInput +0x14
RESET_TIME = 0x00DA1A18          # Com_LocalClient_LastInput_Set's static


def client(lc=0, controller=0, last=0, reset=None):
    def run(w):
        w.write(CLIENT_GAME_STATES, struct.pack("<iiiiii", 0x11, lc, controller, 7, 3, last))
        if reset is not None:
            w.u32(RESET_TIME, reset & 0xFFFFFFFF)
    return run


def now(t, **kw):
    base = client(**kw)

    def run(w):
        STATE["now"] = t
        base(w)
    return run


ASSERT = {"Assert_MyHandler": lambda mu: 1}
CASES = [
    {"name": "Com_LocalClient_GetControllerIndex", "ret": "int", "builds": SERVER, "fakes": ASSERT,
     # the bounds assert's index and count aren't compared: the other assert passes no arguments there
     "observe": {"Assert_MyHandler": "siiss"},
     "inputs": [{"args": [0], "setup": client(0, 0)}, {"args": [0], "setup": client(0, 3)},
                {"args": [0], "setup": client(0, -1)},
                # a record that names another local client: asserts, then answers no controller
                {"args": [0], "setup": client(2, 3)},
                # past MAX_LOCAL_CLIENTS: the bounds assert, then the next record (which names local client 0)
                {"args": [1], "setup": client(0, 5)}]},
    {"name": "Com_LocalClient_LastInput_Get", "ret": "int", "builds": SERVER,
     "inputs": [{"args": [0], "setup": client(last=v)} for v in range(4)]},
    {"name": "Com_LocalClient_LastInput_Set", "ret": "void", "builds": SERVER,
     "fakes": {"Sys_Milliseconds": lambda mu: STATE.get("now", 0), "CG_UpdateVehicleBindings": lambda mu: None},
     "observe": {"CG_UpdateVehicleBindings": "i"},
     "inputs": [{"args": [0, new], "setup": now(t, last=old, reset=reset)}
                for old in range(4) for new in range(4)
                for t, reset in ((1000, 0), (1000, 1100), (1000, 1000), (1000, 900), (5, -7))]},
]
