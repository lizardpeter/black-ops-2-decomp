"""qcommon/common.cpp (server; the Store build inlines Com_ViewScaleMsec, covered by CL_CreateCmd's Store test)."""
import random
import struct

SERVER = ["CoDMPServer_PC"]
SLOWMO = 0x01E29184  # g_slowmoCommon
NAN, INF = float("nan"), float("inf")


def slowmo(cur, enable, start, end, start_msec, end_msec):
    """The view fields of SlowMotionCommon (viewTimescale +0x1C ... viewEndMsec +0x34)."""
    data = struct.pack("<fB3xiffii", cur, enable, 0, start, end, start_msec, end_msec)
    return lambda w: w.write(SLOWMO + 0x1C, data)


def view_scale_inputs():
    """Ramps up and down, reaching and passing their end, equal ends, no duration, NaN and infinite values, results
    below the 0.001 floor, and the effect turned off."""
    rng = random.Random(6125)
    ramps = [(1.0, 1, 1.0, 0.25, 0, 500), (0.3, 1, 1.0, 0.25, 0, 500), (0.26, 1, 1.0, 0.25, 0, 500),
             (0.25, 1, 0.25, 1.0, 100, 600), (0.99, 1, 0.25, 1.0, 100, 600), (0.5, 1, 0.5, 0.5, 0, 100),
             (0.5, 1, 0.25, 1.0, 7, 7), (0.5, 0, 0.25, 1.0, 0, 100), (0.0, 0, 0.0, 1.0, 0, 100),
             (1e-6, 0, 0.0, 1.0, 0, 100), (NAN, 1, 0.25, 1.0, 0, 100), (0.5, 1, NAN, 1.0, 0, 100),
             (0.5, 1, 1.0, NAN, 0, 100), (0.5, 1, 1.0, 0.25, 0, 100), (NAN, 1, 1.0, 0.25, 0, 100),
             (0.5, 1, 0.25, 1.0, 100, 0), (INF, 1, 0.25, 1.0, 0, 100), (0.5, 1, -INF, INF, 0, 100),
             (NAN, 0, 0.25, 1.0, 0, 100), (-1.0, 0, 0.0, 0.0, 0, 0)]
    for _ in range(60):
        ramps.append((rng.uniform(0, 2), rng.randrange(2), rng.uniform(0, 2), rng.uniform(0, 2), rng.randrange(-50, 50),
                      rng.randrange(-50, 2000)))
    secs = (0.016, 0.0, 0.25, 1e-5, -0.016, NAN)
    return [{"args": [("f", sec)], "setup": slowmo(*r)} for r in ramps for sec in secs]


CASES = [
    {"name": "Com_ViewScaleMsec", "ret": "float", "builds": SERVER, "inputs": view_scale_inputs()},
]
