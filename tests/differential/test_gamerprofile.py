"""game/gamerprofile.cpp (server; the Store build inlines GamerProfile_GetFloat, covered by CL_MouseMove's Store test).

GamerProfile_GetInt is faked: the float getter returns its bits, NaNs and denormals included.
"""
import struct

STATE = {}
SERVER = ["CoDMPServer_PC"]


def bits(x):
    def run(w):
        STATE["bits"] = x
    return run


CASES = [
    {"name": "GamerProfile_GetFloat", "ret": "float", "builds": SERVER,
     "fakes": {"GamerProfile_GetInt": lambda mu: STATE["bits"]}, "observe": {"GamerProfile_GetInt": "ii"},
     "inputs": [{"args": [s, c], "setup": bits(b)} for s, c in ((0x29, 0), (3, -1))
                for b in (0, 0x40A00000, 0xBF800000, 0x7FC00000, 0x7F800001, 0x00000001, 0x7F800000, 0x80000000)]},
]
