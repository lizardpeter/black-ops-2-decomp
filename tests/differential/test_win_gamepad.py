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


# -- GPad_StopRumbles: XInputSetState (by ordinal, XINPUT1_3 #3) logs its controller and motor speeds
from harness import OBJECTS  # noqa: E402

LOG = OBJECTS + 0x1F0000


def xinput_set_state(m, mu):
    esp = mu.reg_read(__import__("unicorn").x86_const.UC_X86_REG_ESP)
    port, vibration = struct.unpack("<II", bytes(mu.mem_read(esp + 4, 8)))
    n = struct.unpack("<I", bytes(mu.mem_read(LOG, 4)))[0]
    m.program_write(LOG + 4 + 8 * n, struct.pack("<II", port, struct.unpack("<I", bytes(mu.mem_read(vibration, 4)))[0]))
    m.program_write(LOG, struct.pack("<I", n + 1))
    mu.reg_write(__import__("unicorn").x86_const.UC_X86_REG_EAX, 0)


def rumbling(enabled):
    def run(w):
        w.write(S_GAMEPADS, bytes([enabled]) + bytes(0x4F) + struct.pack("<HH", 0x1234, 0xFFFF) + bytes(0x28))
        w.write(LOG, struct.pack("<I", 0))
    return run


CASES.append({"name": "GPad_StopRumbles", "ret": "void", "builds": SERVER,
              "imports": {"xinput1_3.dll!#3": (xinput_set_state, 2)},
              "fakes": {"Assert_MyHandler": lambda mu: 1}, "observe": {"Assert_MyHandler": "siissii"},
              "inputs": [{"args": [port], "setup": rumbling(e)} for e in (0, 1) for port in (0, 1)]})
