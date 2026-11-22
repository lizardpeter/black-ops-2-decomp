"""gfx_d3d/r_init.cpp (Store only: the server's renderer has no device code)."""
import struct

from harness import OBJECTS

STORE = ["t6mp_msstore"]
R_AA_SAMPLES = 0x1427E7158  # const dvar_t *
DVAR = OBJECTS + 0x1000     # a dvar_t (x64: type +0x18, current +0x20)
TEXT = OBJECTS + 0x1100
DX_MULTISAMPLE = 0x14282E538


def aa(dvar_type, value):
    """r_aaSamples as a dvar of the given type (1 bool, 2 float, 6 int, 7 enum, 8 string), or none."""
    def run(w):
        if dvar_type is None:
            w.u64(R_AA_SAMPLES, 0)
            return
        w.u64(R_AA_SAMPLES, DVAR)
        w.write(DVAR + 0x18, struct.pack("<i", dvar_type))
        if dvar_type == 1:
            w.write(DVAR + 0x20, bytes([value]) + bytes(7))
        elif dvar_type == 2:
            w.write(DVAR + 0x20, struct.pack("<f", value) + bytes(4))
        elif dvar_type in (6, 7):
            w.write(DVAR + 0x20, struct.pack("<iI", value, 0xDEAD))
        else:
            w.write(TEXT, value.encode() + b"\0")
            w.u64(DVAR + 0x20, TEXT)
        w.write(DX_MULTISAMPLE, struct.pack("<II", 0x77, 0x88))
    return run


def window(hwnd, width, height):
    return struct.pack("<QiB3xiiiiii", hwnd, 60, 1, 5, 6, 1280, 720, width, height) + bytes(4)


AA = [(None, 0), (6, 0), (6, 2), (6, 4), (6, 8), (6, 16), (6, 17), (6, -1), (7, 16), (1, 1), (2, 16.9), (2, 4.0),
      (8, "16"), (8, " 8x"), (8, "")]

CASES = [
    {"name": "sub_14071E360", "ret": "void", "builds": STORE,
     "inputs": [{"args": [bytes([0xCC]) * 0x48, window(hwnd, wid, hei)], "setup": aa(t, v)}
                for t, v in AA for hwnd, wid, hei in ((0x50A0B0C0D, 2560, 1440), (0, 0, 0), (1, -1, 7))]},
]
