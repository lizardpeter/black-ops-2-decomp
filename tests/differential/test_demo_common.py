"""demo/demo_common.cpp (server; the Store build inlines Demo_IsPlaying, covered by CL_MouseMove's Store test)."""
import struct

SERVER = ["CoDMPServer_PC"]
DEMO = 0x080D3580  # demoMain: state at +0x84


def state(v):
    return lambda w: w.write(DEMO + 0x84, struct.pack("<i", v))


CASES = [
    {"name": "Demo_IsPlaying", "ret": "bool", "builds": SERVER,
     "inputs": [{"args": [], "setup": state(v)} for v in (0, 1, 2, 3, 0x102)]},
]
