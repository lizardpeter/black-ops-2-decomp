"""client_mp/cl_cgame_mp.cpp (server; the Store build inlines CL_ClampViewAngle, covered by CL_CreateCmd's Store
test)."""
import struct

from harness import OBJECTS

SERVER = ["CoDMPServer_PC"]
CL_MAX_LOCAL_CLIENTS, CLIENTS_PTR = 0x01237580, 0x01237588
VIEWANGLES = 0x2C8C  # clientActive_t.viewangles
NAN, INF = float("nan"), float("inf")


def angles(v, max_clients=1):
    def run(w):
        w.u32(CL_MAX_LOCAL_CLIENTS, max_clients)
        w.u32(CLIENTS_PTR, OBJECTS)
        w.write(OBJECTS + VIEWANGLES, struct.pack("<3f", *v))
    return run


def clamp_inputs():
    out = []
    for value in (0.0, 45.0, 90.0, 90.00001, -90.0, -120.0, 359.0, NAN, INF, -INF, -0.0):
        for lo, hi in ((-90.0, 90.0), (-88.0, 88.0), (0.0, 60.0), (-60.0, 60.0), (10.0, 10.0), (60.0, 0.0),
                       (NAN, 1.0), (-1.0, NAN)):
            for angle in (0, 1, 2):
                v = [7.0, 8.0, 9.0]
                v[angle] = value
                out.append({"args": [0, angle, ("f", lo), ("f", hi)], "setup": angles(v)})
    # out of range: the bounds assert (its text spells ARRAY_COUNT out), then the write beside viewangles
    for angle in (3, -1):
        out.append({"args": [0, angle, ("f", -1.0), ("f", 1.0)], "setup": angles([7.0, 8.0, 9.0])})
    return out


CASES = [
    {"name": "CL_ClampViewAngle", "ret": "void", "builds": SERVER, "fakes": {"Assert_MyHandler": lambda mu: 1},
     "observe": {"Assert_MyHandler": "siiss"},
     "inputs": clamp_inputs()},
]
