"""client/splitscreen.cpp (server; the Store build inlines these, covered by CL_Input's Store test)."""
import struct

SERVER = ["CoDMPServer_PC"]
CLIENT_UI = 0x01237050  # clientUIActives[]: flags at +0, 0x458 apart


def flags(*values):
    def run(w):
        for i, v in enumerate(values):
            w.write(CLIENT_UI + 0x458 * i, struct.pack("<i", v))
    return run


FAKES = {"Assert_MyHandler": lambda mu: 1}
SEEN = {"Assert_MyHandler": "siiss"}
# past the one local client the asserts fire and the reads go on into the next clientUIActive_t's bytes
FLAGS = ((0, 0, 0), (1, 0, 0), (0, 1, 0), (3, 1, 1), (2, 2, 1), (0, 0, 1))

CASES = [
    {"name": "CL_LocalClient_IsActive", "ret": "bool", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [lc], "setup": flags(*f)} for lc in (0, 1, 2) for f in FLAGS]},
    {"name": "CL_IsFirstActiveLocalClient", "ret": "bool", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [{"args": [lc], "setup": flags(*f)} for lc in (0, 1, 2, -1) for f in FLAGS]},
    {"name": "CL_SplitscreenEnabled", "ret": "bool", "builds": SERVER, "inputs": [[]]},
]
