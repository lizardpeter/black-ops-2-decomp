"""win32/win_gamepad.cpp (server; the Store build inlines GPad_InUse and GPad_IsActive, covered by CL_MouseMove's and
CL_CreateCmd's Store tests)."""
import struct

SERVER = ["CoDMPServer_PC"]
CLIENT_GAME_STATES = 0x01D77360
S_GAMEPADS = 0x07AB7678  # GamePad[1]: enabled at +0


def last_input(v):
    return lambda w: w.write(CLIENT_GAME_STATES + 0x14, struct.pack("<i", v))


def gamepad(enabled):
    return lambda w: w.write(S_GAMEPADS, bytes([enabled]) + bytes(0x7B))


CASES = [
    {"name": "GPad_IsActive", "ret": "bool", "builds": SERVER,
     "inputs": [{"args": [0], "setup": gamepad(v)} for v in (0, 1)]},
    {"name": "GPad_InUse", "ret": "bool", "builds": SERVER,
     "inputs": [{"args": [0], "setup": last_input(v)} for v in (0, 1, 2, 3, -1)]},
]
