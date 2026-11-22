"""gfx_d3d/r_state.cpp: R_DX11_ReleaseSamplerStates (server; the Store build inlines it into RB_SwapBuffers). Each
sampler state is a com.py object, so every Release is compared, and the table's cleared slots are compared writes.
"""
import random
import struct

from com import ComModel

SERVER = ["CoDMPServer_PC"]
S_SAMPLER_STATES = 0x08503D18


def table(seed, density):
    def run(w):
        r = random.Random(seed)
        model = ComModel({("ID3D11State", 2 * 8): ""})
        model.install(w.m)
        for i in range(256):
            w.write(S_SAMPLER_STATES + 4 * i,
                    struct.pack("<I", model.new("ID3D11State") if r.random() < density else 0))
    return run


CASES = [
    {"name": "R_DX11_ReleaseSamplerStates", "ret": "void", "builds": SERVER,
     "inputs": [{"args": [], "setup": table(s, d)} for s, d in enumerate((0.0, 0.05, 0.5, 1.0))]},
]
