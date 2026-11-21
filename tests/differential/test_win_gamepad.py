"""win32/win_gamepad.cpp (server; the Store build inlines GPad_InUse, covered by CL_MouseMove's Store test)."""
import struct

SERVER = ["CoDMPServer_PC"]
CLIENT_GAME_STATES = 0x01D77360


def last_input(v):
    return lambda w: w.write(CLIENT_GAME_STATES + 0x14, struct.pack("<i", v))


CASES = [
    {"name": "GPad_InUse", "ret": "bool", "builds": SERVER,
     "inputs": [{"args": [0], "setup": last_input(v)} for v in (0, 1, 2, 3, -1)]},
]
