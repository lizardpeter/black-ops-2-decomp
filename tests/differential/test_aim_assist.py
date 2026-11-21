"""aim_assist/aim_assist.cpp: the mouse's entry point (server; the Store build inlines it into CL_MouseMove, whose
Store test covers it there).

aaGlobArray is the server's own global: the setup sets whether aim assist is initialized and its copy of the weapon
state; the write of prev_button_bits is compared. The three helpers it calls are faked and their calls observed.
"""
import struct

SERVER = ["CoDMPServer_PC"]
AA_GLOB = 0x010D2820  # AimAssistGlobals[1]: ps.weaponstate +0x48, initialized +0x6C, prev_button_bits +0x12B0


def aim_input(pitch, yaw, bits=(0x12345678, 0x9ABCDEF0)):
    return struct.pack("<9f2Ii I f", 0.016, pitch, 0.0, 0.0, yaw, 0.0, 0.0, 0.0, 0.0, *bits, 0, 0, 0.0)


def glob(initialized, weaponstate):
    def run(w):
        w.u8(AA_GLOB + 0x6C, initialized)
        w.u32(AA_GLOB + 0x48, weaponstate)
        w.write(AA_GLOB + 0x12B0, b"\xAA" * 8)
    return run


CASES = [
    {"name": "AimAssist_UpdateMouseInput", "ret": "void", "builds": SERVER,
     "fakes": {"AimAssist_UpdateTweakables": lambda mu: None, "AimAssist_UpdateAdsLerp": lambda mu: None,
               "AimAssist_ApplyAutoMelee": lambda mu: None, "Assert_MyHandler": lambda mu: 1},
     # UpdateAdsLerp takes input in EDI (records/conventions.tsv): its call is observed, not its argument
     "observe": {"AimAssist_UpdateTweakables": "q", "AimAssist_UpdateAdsLerp": "", "AimAssist_ApplyAutoMelee": "qq",
                 "Assert_MyHandler": "siiss"},
     "inputs": [{"args": [aim_input(p, y), bytes(16)], "setup": glob(init, ws)}
                for p, y in ((12.5, -170.25), (-89.0, 359.0))
                for init in (0, 1)
                for ws in (0, 0x2C, 0x37, 0x38, 0x39, 0x3A, 0x3B, -1)]},
]
