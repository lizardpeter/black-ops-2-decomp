"""universal/mem_largelocal.cpp on the server (the Store build inlines it: tests/replay/verify_ui_motd.py)."""
import struct

SERVER = ["CoDMPServer_PC"]
POS, RIGHT = 0x07A6F440, 0x00DB6278  # g_largeLocalPos, g_largeLocalRightPos (analysis/CoDMPServer_PC/pdb/statics.tsv)
THIS = 0x31000100
STATE = {"main": 1, "server": 0}


def state(pos, right, main=1, server=0, this=None):
    def run(w):
        STATE.update(main=main, server=server)
        w.write(POS, struct.pack("<i", pos))
        w.write(RIGHT, struct.pack("<i", right))
        if this:
            w.write(THIS, struct.pack("<ii", *this))
    return run


FAKES = {"Sys_IsMainThread": lambda mu: STATE["main"], "Sys_IsServerThread": lambda mu: STATE["server"],
         "Assert_MyHandler": lambda mu: 1}
OBSERVE = {"Assert_MyHandler": "sii"}
POSITIONS = [(0, 0x80000), (0x100, 0x7F000), (0x7FFF0, 0x80000), (0x1000, 0x1000), (0x2000, 0x1000)]
THREADS = [(1, 0), (0, 1), (0, 0)]
SIZES = [0, 1, 3, 4, 0x1000, 0x7FFFD, -4]


def inputs(args_of, with_this=False):
    out = []
    for pos, right in POSITIONS:
        for main, server in THREADS:
            for a in args_of(pos, right):
                out.append({"args": a, "setup": state(pos, right, main, server,
                                                     (a[1], a[2]) if with_this else None)})
    return out


def this_inputs(pos, right):
    return [[THIS, start, size] for start in (pos, right, 0x40) for size in (0x1000, 5)]


CASES = [
    {"name": n, "ret": r, "builds": SERVER, "fakes": FAKES, "observe": OBSERVE, "inputs": inputs(a)}
    for n, r, a in [
        ("LargeLocalBegin", "int", lambda p, r: [[s] for s in SIZES]),
        ("LargeLocalBeginRight", "int", lambda p, r: [[s] for s in SIZES]),
        ("LargeLocalEnd", "void", lambda p, r: [[s] for s in (0, p, r, r + 4)]),
        ("LargeLocalEndRight", "void", lambda p, r: [[s] for s in (0, p, r, r + 4)]),
        ("LargeLocalGetBuf", "uint", lambda p, r: [[s, z] for s in (0, 0x1000, r) for z in (0x1000, 5, 0x2000)]),
        ("LargeLocalReset", "void", lambda p, r: [[]]),
        ("??0LargeLocal@@QAE@H@Z", "uint", lambda p, r: [[THIS, s] for s in SIZES]),
    ]
] + [
    {"name": "??1LargeLocal@@QAE@XZ", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": OBSERVE,
     "inputs": [{"args": [THIS], "setup": i["setup"]} for i in inputs(this_inputs, True)]},
    {"name": "?GetBuf@LargeLocal@@QAEPAXXZ", "ret": "uint", "builds": SERVER, "fakes": FAKES, "observe": OBSERVE,
     "inputs": [{"args": [THIS], "setup": i["setup"]} for i in inputs(this_inputs, True)]},
]
