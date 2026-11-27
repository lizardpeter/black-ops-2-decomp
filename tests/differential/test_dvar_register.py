"""universal/dvar.cpp's registration path on the server (the Store build: tests/replay/verify_dvar_register.py).

Only the value lanes a float registration fills are observed: the rest of the value and the domain are whatever the
stack held, in the original as here."""
import struct

SERVER = ["CoDMPServer_PC"]
FIND = "?Dvar_FindMalleableVar@@YAPAUdvar_t@@PBD@Z"
STATE = {"exists": 0}


def exists(on):
    def run(w):
        STATE["exists"] = on
    return run


FAKES = {FIND: lambda mu: 0x31000100 if STATE["exists"] else 0,
         "Dvar_RegisterNew": lambda mu: 0x31000200, "Dvar_Reregister": lambda mu: 0,
         "CanKeepStringPointer": lambda mu: 1, "Assert_MyHandler": lambda mu: 1}
# The server's link-time code generation passes Dvar_FindMalleableVar's and Dvar_RegisterNew's arguments in registers
# from here (the pseudocode's unaff_ESI/EDI), so only which of them is called, and in what order, is compared; the
# arguments are compared in the Store build's test.
OBSERVE = {FIND: "", "Dvar_RegisterNew": "", "Dvar_Reregister": ""}
f = lambda x: ("f", x)

CASES = [
    {"name": "_Dvar_RegisterFloat", "ret": "uint", "builds": SERVER, "fakes": FAKES, "observe": OBSERVE,
     "inputs": [{"args": [b"cg_fovScale\0", f(v), f(0.2), f(2.0), flags, b"\0"], "setup": exists(e)}
                for v in (1.0, 0.5) for flags in (0, 0x80, 0x4000) for e in (0, 1)]},
]
