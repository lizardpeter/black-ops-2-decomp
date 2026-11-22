"""universal/dvar.cpp: the typed getters, on the server and the Store build.

Each input is a dvar_t written into the compared object region with the build's layout (x86: type at 0x10, value at
0x18; x64: 0x18 and 0x20). The Store build calls the CRT's atoi and atof as imports, which the harness otherwise
stubs: they are modelled here.
"""
import struct

from unicorn import x86_const as x86

from harness import OBJECTS

BOTH = ["CoDMPServer_PC", "t6mp_msstore"]
SERVER = ["CoDMPServer_PC"]
DVAR = OBJECTS + 0x1000
NAME, STRING = OBJECTS + 0x2000, OBJECTS + 0x2100
BOOL, FLOAT, INT, ENUM, STRING_T, COLOR = 1, 2, 6, 7, 8, 9


def write_dvar(type_, value=0, flags=0, string=None):
    """value: an int or float for the current value, or ignored for a string dvar (string)."""
    def run(w):
        x64 = w.m.x64
        w.write(NAME, b"cl_test\0")
        if string is not None:
            w.write(STRING, string.encode() + b"\0")
        ptr = "<Q" if x64 else "<I"
        w.write(DVAR, struct.pack(ptr, NAME))
        w.write(DVAR + (0x14 if x64 else 0xC), struct.pack("<I", flags))
        w.write(DVAR + (0x18 if x64 else 0x10), struct.pack("<i", type_))
        current = DVAR + (0x20 if x64 else 0x18)
        if string is not None:
            w.write(current, struct.pack(ptr, STRING))
        elif isinstance(value, float):
            w.write(current, struct.pack("<f", value))
        else:
            w.write(current, struct.pack("<i", value))
    return run


def crt_arg_string(mu):
    p = mu.reg_read(x86.UC_X86_REG_RCX)
    out = b""
    while len(out) < 64:
        c = bytes(mu.mem_read(p + len(out), 1))
        if c == b"\0":
            break
        out += c
    return out.decode("latin-1")


def leading_number(s, kind):
    import re
    m = re.match(r"\s*[+-]?(\d+\.?\d*(e[+-]?\d+)?|\.\d+)" if kind is float else r"\s*[+-]?\d+", s, re.I)
    return kind(m.group(0)) if m else kind(0)


def atoi(m, mu):
    v = leading_number(crt_arg_string(mu), int)
    mu.reg_write(x86.UC_X86_REG_RAX, max(-2**31, min(2**31 - 1, v)) & 0xFFFFFFFF)


def atof(m, mu):
    v = leading_number(crt_arg_string(mu), float)
    mu.reg_write(x86.UC_X86_REG_XMM0, struct.unpack("<Q", struct.pack("<d", v))[0])


IMPORTS = {"api-ms-win-crt-convert-l1-1-0.dll!atoi": (atoi, 0), "api-ms-win-crt-convert-l1-1-0.dll!atof": (atof, 0)}
FAKES = {"Com_PrintWarning": lambda mu: None, "Assert_MyHandler": lambda mu: 1}
SEEN = {"Com_PrintWarning": "isss", "Assert_MyHandler": "siisss"}

STRINGS = ["0", "1", "17", "-3", "2.75", " 9x", "", "abc", "1e3", "-0.5"]


def inputs(numbers, float_result=False):
    out = [{"args": [0]}]
    for t, vals in numbers:
        for v in vals:
            inp = {"args": [DVAR], "setup": write_dvar(t, v)}
            if float_result and t == INT and abs(v) > 1 << 24:
                # The server returns fild's result unrounded in ST0 and clang can't: at -O0 a function with several
                # returns passes its result through a float stack slot (docs/verification.md). Store only.
                inp["builds"] = ["t6mp_msstore"]
            out.append(inp)
    for s in STRINGS:
        for flags in (0, 1 << 14):
            out.append({"args": [DVAR], "setup": write_dvar(STRING_T, string=s, flags=flags)})
    return out


NUMBERS = [(BOOL, [0, 1, 0x101]), (INT, [0, 7, -2, 2**31 - 1]), (ENUM, [3]),
           (FLOAT, [0.0, 2.9, -2.9, 1e9, float("nan")]), (COLOR, [0x11223344])]

CASES = [
    {"name": "Dvar_GetBool", "ret": "bool", "builds": BOTH, "imports": IMPORTS, "fakes": FAKES, "observe": SEEN,
     "inputs": inputs(NUMBERS)},
    {"name": "Dvar_GetInt", "ret": "int", "builds": BOTH, "imports": IMPORTS, "fakes": FAKES, "observe": SEEN,
     "inputs": inputs(NUMBERS)},
    {"name": "Dvar_GetFloat", "ret": "float", "builds": BOTH, "imports": IMPORTS, "fakes": FAKES, "observe": SEEN,
     "inputs": inputs(NUMBERS, float_result=True)},
    {"name": "Dvar_StringToBool", "ret": "bool", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [[s.encode() + b"\0"] for s in STRINGS]},
    {"name": "Dvar_StringToInt", "ret": "int", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [[s.encode() + b"\0"] for s in STRINGS]},
    {"name": "Dvar_StringToFloat", "ret": "float", "builds": SERVER, "fakes": FAKES, "observe": SEEN,
     "inputs": [[s.encode() + b"\0"] for s in STRINGS]},
]


# -- the latched value (server: the Store build inlines both into RB_SwapBuffers). Dvar_ValuesEqual runs for real
# (its type comes in ECX: records/conventions.tsv), Dvar_SetVariant is observed: the dvar, the latched value's four
# dwords and the source.
def write_latched(type_, current, latched):
    def run(w):
        w.write(NAME, b"cl_test\0")
        w.write(DVAR, struct.pack("<I", NAME))
        w.write(DVAR + 0x10, struct.pack("<i", type_))
        for off, v in ((0x18, current), (0x28, latched)):
            w.write(DVAR + off, (struct.pack("<f", v) if isinstance(v, float) else struct.pack("<i", v)) + bytes(12))
    return run


LATCHED = [(BOOL, 0, 0), (BOOL, 0, 1), (BOOL, 1, 1), (INT, 5, 5), (INT, 5, -5), (INT, 0, 1 << 20),
           (ENUM, 2, 2), (ENUM, 2, 3), (FLOAT, 1.5, 1.5), (FLOAT, 1.5, 1.25), (FLOAT, 0.0, -0.0)]

CASES += [
    {"name": "Dvar_HasLatchedValue", "ret": "bool", "builds": SERVER,
     "inputs": [{"args": [DVAR], "setup": write_latched(*t)} for t in LATCHED]},
    {"name": "Dvar_MakeLatchedValueCurrent", "ret": "void", "builds": SERVER,
     "fakes": {"Dvar_SetVariant": lambda mu: None}, "observe": {"Dvar_SetVariant": "qiiiii"},
     "inputs": [{"args": [DVAR], "setup": write_latched(*t)} for t in LATCHED]},
]


# -- the setters (server; inlined in the Store build, where R_CreateDeviceInternal's test covers them). The new
# value Dvar_SetVariant gets is logged as the dvar's type reads it: a bool's byte (the other bytes are whatever was
# on the stack) or a string's text ("1" / "0", at different addresses in the two images).
SETTER = {}


def write_setter(type_, flags=0, named=True):
    def run(w):
        SETTER["m"] = w.m
        w.write(NAME, b"cl_test\0")
        w.write(DVAR, struct.pack("<I", NAME if named else 0))
        w.write(DVAR + 0xC, struct.pack("<I", flags))
        w.write(DVAR + 0x10, struct.pack("<i", type_))
    return run


def set_variant(mu):
    m = SETTER["m"]
    sp = mu.reg_read(x86.UC_X86_REG_ESP)
    dvar, value, *_, source = struct.unpack("<6I", bytes(mu.mem_read(sp + 4, 24)))
    type_ = struct.unpack("<i", bytes(mu.mem_read(dvar + 0x10, 4)))[0]
    m.observed.append(("Dvar_SetVariant", dvar, value & 0xFF if type_ == BOOL else m.read_cstr(value), source))


SETTERS = [(BOOL, 0, True), (STRING_T, 1 << 14, True), (STRING_T, 0, True), (INT, 0, True), (BOOL, 0, False)]
ASSERTS = {"Assert_MyHandler": lambda mu: 1}

CASES += [
    {"name": "Dvar_SetBoolFromSource", "ret": "void", "builds": SERVER,
     "fakes": dict(ASSERTS, Dvar_SetVariant=set_variant), "observe": {"Assert_MyHandler": "siiss"},
     "inputs": [{"args": [DVAR, v, src], "setup": write_setter(*t)} for t in SETTERS for v in (0, 1)
                for src in (0, 2)]},
    {"name": "Dvar_SetBool", "ret": "void", "builds": SERVER,
     "fakes": dict(ASSERTS, Dvar_SetVariant=set_variant), "observe": {"Assert_MyHandler": "siiss"},
     "inputs": [{"args": [DVAR, v], "setup": write_setter(*t)} for t in SETTERS for v in (0, 1)]},
    {"name": "Dvar_SetInt", "ret": "void", "builds": SERVER,
     "fakes": {"Dvar_SetIntFromSource": lambda mu: None}, "observe": {"Dvar_SetIntFromSource": "qii"},
     "inputs": [{"args": [DVAR, v], "setup": write_setter(INT)} for v in (0, 1, -1, 16, 1 << 30)]},
]
