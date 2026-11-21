"""universal/q_shared.cpp."""
import random
import struct

from unicorn import x86_const as x86

from harness import SCRATCH, STACK, STACK_SIZE

rng = random.Random(1999)
STATE = {}

# Fixed places in scratch memory (cleared before every call): Writer.alloc hands out the start of it.
VA_INFO = SCRATCH + 0x80000          # what Sys_GetValue(1) returns: this thread's va_info_t
LAYOUT = SCRATCH + 0x90000           # argument structures (field lists, pointer arrays)
CALLBACKS = SCRATCH + 0xF0000        # addresses of function-pointer arguments, each faked below
PARSE_STR_CPY, PARSE_SPECIAL, PARSE_MERGE = CALLBACKS, CALLBACKS + 0x10, CALLBACKS + 0x20

# The statics behind BigShort and friends (analysis/CoDMPServer_PC/pdb/statics.tsv) and the functions Swap_Init
# points them at, in CoDMPServer_PC.
STATICS = {"_BigShort": 0x07A8DCC0, "_LittleShort": 0x07A8DCBC, "_BigLong": 0x07A8DCC4, "_LittleLong": 0x07A8DCAC,
           "_BigLong64": 0x07A8DCB0, "_LittleFloatWrite": 0x07A8DCB8}
SHORT_SWAP, LONG_SWAP, LONG64_SWAP, NO_SWAP = 0x0078DCC0, 0x0078DCF0, 0x0078DD20, 0x00834010


def returns(value):
    return lambda mu: value


def from_state(key):
    return lambda mu: STATE[key]


def state(**kw):
    def setup(w):
        STATE.update(kw)
    return setup


ASSERT_FAKE = {"Assert_MyHandler": returns(1)}  # an assert that is ignored: the code carries on, as in the original
ASSERT_SEEN = {"Assert_MyHandler": "siiss"}     # file, line, type, format, first argument (a string in this file)


def cstr(s):
    return (s.encode("latin1") if isinstance(s, str) else s) + b"\0"


def ints(values):
    return [[v] for v in values]


SMALL_INTS = list(range(-300, 300)) + [-2**31, 2**31 - 1, 0x92, -0x6E, 0x192]

PATHS = ["", "a", "file", "file.txt", "dir/file", "dir\\file.ext", "dir.d/file", "dir.d\\file", "a.b.c", ".hidden",
         "dir/", "dir\\", "/", "trailing.", "x/y/z.tar.gz", "maps/mp/mp_raid.d3dbsp", "c:\\t6\\code\\q.cpp",
         "no/ext.here/", "a/./b", "..", "\x80\xff.\x01"]

STRINGS = ["", "a", "A", "abc", "ABC", "abd", "ab", "abcd", "Hello World", "hello world", "HELLO WORLD!", "^1Red",
           "[{+attack}]", "\x7f\x80\xfe\xff", "zz", "@[`{", "0123456789", "mixed CASE 123", "\x92quote\x92"]


def string_pairs():
    pairs = [(a, b) for a in STRINGS for b in STRINGS[:8]]
    for _ in range(150):
        a = "".join(chr(rng.choice(b"aAbB\x80\xffz[{^0")) for _ in range(rng.randrange(6)))
        b = "".join(chr(rng.choice(b"aAbB\x80\xffz[{^0")) for _ in range(rng.randrange(6)))
        pairs.append((a, b))
    return pairs


def random_text(n, alphabet="abcXYZ09 ^^[{}\\;\"./*?\x01\x7f\x80\x92\xff"):
    return "".join(rng.choice(alphabet) for _ in range(n))


# -- byte order --------------------------------------------------------------------------------------------------

def through(static, target, inputs):
    """Cases for a wrapper that calls through one of Swap_Init's function pointers."""
    return [{"args": a, "setup": (lambda w, s=static, t=target: w.ptr(STATICS[s], t))} for a in inputs]


def floats():
    vals = [0.0, -0.0, 1.0, -1.0, 0.5, 1e-45, 3.4e38, float("inf"), float("-inf"), float("nan"), 123.456, -2.5e-20]
    return vals + [struct.unpack("<f", struct.pack("<I", rng.getrandbits(32)))[0] for _ in range(100)]


def words(width):
    return [0, 1, (1 << width) - 1, 0x0123456789ABCDEF & ((1 << width) - 1)] + [rng.getrandbits(width) for _ in range(100)]


def signed(v, width):
    return v - (1 << width) if v >> (width - 1) else v


# -- strings and info strings -----------------------------------------------------------------------------------

INFO = ["", "\\", "\\name\\player", "\\name\\player\\rate\\25000", "name\\player\\rate\\25000",
        "\\a\\1\\b\\2\\c\\3", "\\A\\upper\\a\\lower", "\\key\\", "\\key", "\\\\empty\\x\\y", "\\k\\v\\\\",
        "\\sv_hostname\\^1My ^7Server\\g_gametype\\tdm\\mapname\\mp_raid", "\\x\\1\\x\\2"]
KEYS = ["name", "NAME", "rate", "a", "b", "c", "x", "key", "", "missing", "sv_hostname", "mapname", "k"]


def info_value_inputs():
    out = []
    for s in INFO:
        for k in KEYS:
            for thread in ("main", "server", "render"):
                out.append({"args": [cstr(s), cstr(k)], "setup": state(thread=thread)})
    out.append({"args": [0, cstr("a")], "setup": state(thread="main")})
    out.append({"args": [cstr("\\a\\b"), 0], "setup": state(thread="main")})
    long = "\\" + "k" * 3000 + "\\" + "v" * 5000 + "\\key\\found"
    out.append({"args": [cstr(long), cstr("key")], "setup": state(thread="server")})
    return out


THREAD_FAKES = {
    "Sys_IsMainThread": lambda mu: STATE["thread"] == "main",
    "Sys_IsServerThread": lambda mu: STATE["thread"] == "server",
    "Sys_IsRenderThread": lambda mu: STATE["thread"] == "render",
}


def remove_inputs(size):
    out = []
    for s in INFO + ["\\name\\" + "p" * 900, "\\" + "k" * 50 + "\\v\\name\\x"]:
        for k in KEYS + ["a\\b"]:
            buf = cstr(s).ljust(size + 16, b"\xcc")
            out.append([buf, cstr(k)])
    return out


def set_inputs(size):
    out = []
    values = ["", "value", "with\\backslash", "semi;colon", 'quote"d', "\\;\"", "^1colour", "x" * 40]
    for s in INFO[:8]:
        for k in ["name", "new", "a", "bad\\key", "bad;key", 'bad"key', ""]:
            for v in values:
                out.append([cstr(s).ljust(size + 64, b"\xcc"), cstr(k), cstr(v)])
    # near the limits: a value that just fits, one past it, and an infostring already full
    out.append([cstr("\\a\\" + "b" * (size - 12)).ljust(size + 64, b"\xcc"), cstr("k"), cstr("vvvvv")])
    out.append([cstr("\\a\\" + "b" * (size - 9)).ljust(size + 64, b"\xcc"), cstr("k"), cstr("vvvvv")])
    out.append([cstr("\\a\\" + "b" * (size - 5)).ljust(size + 64, b"\xcc"), cstr("k"), cstr("v")])
    out.append([cstr("x" * size).ljust(size + 64, b"\xcc"), cstr("k"), cstr("v")])
    out.append([cstr("\\a\\b").ljust(size + 64, b"\xcc"), cstr("k"), cstr("q" * (size + 10))])
    out.append({"args": [cstr("\\a\\b").ljust(size + 64, b"\xcc"), cstr("k"), 0]})
    return out


SET_FAKES = {**ASSERT_FAKE, "Com_Printf": returns(None), "Sys_GetValue": returns(VA_INFO)}
SET_SEEN = {**ASSERT_SEEN, "Com_Printf": "is"}


# -- config strings to structs -----------------------------------------------------------------------------------

class Layout:
    """Builds structures in scratch memory at LAYOUT, deterministic, so arguments can point into them."""

    def __init__(self):
        self.data = bytearray()

    def put(self, raw):
        va = LAYOUT + len(self.data)
        self.data += raw
        while len(self.data) % 16:
            self.data += b"\0"
        return va

    def cstr(self, s):
        return self.put(cstr(s))

    def setup(self, extra=None):
        data = bytes(self.data)

        def run(w):
            w.write(LAYOUT, data)
            if extra:
                extra(w)
        return run


FIELD_TYPES = [(1, "s1024"), (2, "s64"), (3, "s256"), (4, "int"), (5, "uint"), (6, "bool"), (7, "qbool"),
               (8, "float"), (9, "ms"), (10, "fx"), (11, "xmodel"), (12, "material"), (13, "stream"), (14, "preset"),
               (15, "sstr"), (16, "tracer"), (17, "alias"), (0, "custom_string"), (18, "special"), (19, "special2")]
FIELD_VALUES = ["", "0", "1", "-1", "42", "4294967295", "4294967296", "1.5", "-0.001", "2.71828", "abc", "1e9",
                "0x10", "  7", "path/to/model", "x" * 300]


def config_inputs():
    out = []
    for n in range(12):
        lay = Layout()
        picked = rng.sample(FIELD_TYPES, rng.randrange(1, len(FIELD_TYPES)))
        info, fields = "", b""
        for i, (ftype, name) in enumerate(picked):
            value = rng.choice(FIELD_VALUES)
            if rng.random() < 0.8:
                info += f"\\{name}\\{value}"
            fields += struct.pack("<Iii", lay.cstr(name), 0x40 * i, ftype)
        field_list = lay.put(fields)
        max_types = rng.choice([0, 19, 20])
        args = [bytes(0x40 * len(picked) + 0x400), field_list, len(picked), cstr(info), max_types, PARSE_SPECIAL,
                PARSE_STR_CPY]
        out.append({"args": args, "setup": lay.setup(state(thread="main", special=n % 2, model=0x1234 * (n % 3)))})
    return out


def merge_special_case(mu):
    """parseMergeSpecialCase(name, values, out, outSize): on success writes a merged value into out (a buffer on the
    caller's stack, so its address is not compared)."""
    if STATE["merge"]:
        esp = mu.reg_read(x86.UC_X86_REG_ESP)
        out = struct.unpack("<I", bytes(mu.mem_read(esp + 12, 4)))[0]
        mu.mem_write(out, b"merged\0")
    return STATE["merge"]


CONFIG_FAKES = {
    **ASSERT_FAKE, **THREAD_FAKES,
    "Sys_GetValue": returns(VA_INFO), "Com_Error": returns(None), "Com_PrintError": returns(None),
    "R_RegisterModel": from_state("model"), "PhysPreset_Register": returns(0x5678),
    "SL_GetLowercaseString": returns(0x1ABCD), "Tracer_Register": returns(0x9ABC), "SND_HashName": returns(0xDEADBEEF),
    PARSE_STR_CPY: returns(None), PARSE_SPECIAL: from_state("special"), PARSE_MERGE: merge_special_case,
}
CONFIG_SEEN = {
    **ASSERT_SEEN, "Com_Error": "is", "R_RegisterModel": "s", "PhysPreset_Register": "s",
    "SL_GetLowercaseString": "si", "Tracer_Register": "s", "SND_HashName": "s", PARSE_STR_CPY: "qs",
    PARSE_SPECIAL: "qsii", PARSE_MERGE: "s", "Com_PrintError": "isssssssss", "Info_SetValueForKey_Big": "qss",
}


def merged_inputs():
    out = []
    names = ["a", "b", "c", "num", "model"]
    for n in range(40):
        lay = Layout()
        fields = b""
        for i, name in enumerate(names):
            fields += struct.pack("<Iii", lay.cstr(name), 4 * i, rng.choice([4, 1, 8, 0]))
        field_list = lay.put(fields)
        sources = []
        for _ in range(3):
            info = "".join(f"\\{k}\\{rng.choice(['', '1', '2', 'x'])}" for k in names if rng.random() < 0.85)
            sources.append(lay.cstr(info))
        buffers = lay.put(struct.pack("<3I", *sources))
        source_names = lay.put(struct.pack("<3I", lay.cstr("base"), lay.cstr("left"), lay.cstr("right")))
        merged = cstr("\\pre\\1").ljust(0x5100, b"\0")
        args = [bytes(0x400), field_list, len(names), cstr("merged_asset"), buffers, source_names, merged, 0,
                PARSE_SPECIAL, PARSE_STR_CPY, PARSE_MERGE]
        out.append({"args": args, "setup": lay.setup(state(thread="main", special=1, merge=n % 2, model=1))})
    return out


# -- maths -------------------------------------------------------------------------------------------------------

def vec(*xs):
    return struct.pack(f"<{len(xs)}f", *xs)


def rand_floats(n, scale=10.0):
    return [rng.uniform(-scale, scale) for _ in range(n)]


def orientation():
    return vec(*rand_floats(12))


def usercmd(bits):
    return struct.pack("<iII", 0, bits >> 32, bits & 0xFFFFFFFF) + bytes(0x44 - 12)


CASES = [
    {"name": "TRACK_q_shared", "ret": "void", "inputs": [[]],
     "fakes": {"track_static_alloc_internal": returns(None)}, "observe": {"track_static_alloc_internal": "qisi"}},
    {"name": "ColorIndex", "ret": "uchar", "inputs": [[v] for v in range(256)]},
    {"name": "Com_GetFilenameSubString", "ret": "cstr", "inputs": [[cstr(p)] for p in PATHS]},
    {"name": "Com_GetExtensionSubString", "ret": "cstr", "inputs": [[cstr(p)] for p in PATHS],
     "fakes": ASSERT_FAKE, "observe": ASSERT_SEEN},
    {"name": "Com_StripExtension", "ret": "void", "inputs": [[cstr(p), bytes(80)] for p in PATHS]},
    {"name": "Com_DefaultExtension", "ret": "void",
     "inputs": [[cstr(p).ljust(80, b"\0"), size, cstr(e)] for p in PATHS for size in (80, 8) for e in (".ff", "")] +
               [[cstr("x" * 70).ljust(80, b"\0"), 80, cstr(".d3dbsp")]]},
    {"name": "BigShort", "ret": "short", "inputs": through("_BigShort", SHORT_SWAP, [[signed(v, 16)] for v in words(16)])},
    {"name": "LittleShort", "ret": "short", "inputs": through("_LittleShort", NO_SWAP, [[signed(v, 16)] for v in words(16)])},
    {"name": "BigLong", "ret": "int", "inputs": through("_BigLong", LONG_SWAP, [[signed(v, 32)] for v in words(32)])},
    {"name": "LittleLong", "ret": "int", "inputs": through("_LittleLong", NO_SWAP, [[signed(v, 32)] for v in words(32)])},
    {"name": "BigLong64", "ret": "uint64", "inputs": through("_BigLong64", LONG64_SWAP, [[("q", v)] for v in words(64)])},
    {"name": "LittleFloatWrite", "ret": "int", "inputs": through("_LittleFloatWrite", NO_SWAP, [[("f", x)] for x in floats()])},
    {"name": "ShortSwap", "ret": "short", "inputs": [[v] for v in range(-2**15, 2**15, 7)] + [[-1], [2**15 - 1]]},
    {"name": "ShortNoSwap", "ret": "short", "inputs": [[v] for v in range(-2**15, 2**15, 7)]},
    {"name": "LongSwap", "ret": "int", "inputs": [[signed(v, 32)] for v in words(32)]},
    {"name": "LongNoSwap", "ret": "int", "inputs": [[signed(v, 32)] for v in words(32)]},
    {"name": "Long64Swap", "ret": "uint64", "inputs": [[("q", v)] for v in words(64)]},
    {"name": "Long64NoSwap", "ret": "uint64", "inputs": [[("q", v)] for v in words(64)]},
    {"name": "FloatWriteNoSwap", "ret": "int", "inputs": [[("f", x)] for x in floats()]},
    {"name": "FloatReadNoSwap", "ret": "float", "inputs": [[signed(v, 32)] for v in words(32)] +
     [[0x7F800000], [0x7FC00000], [0x00000001], [-0x80000000]]},
    {"name": "Swap_Init", "ret": "void", "inputs": [{"args": [], "setup": state(console=c)} for c in (0, 1)],
     "fakes": {"Com_IsClientConsole": from_state("console")}},
    {"name": "I_atoui", "ret": "uint",
     "inputs": [[0]] + [[cstr(s)] for s in ["", "0", "1", "42", "4294967295", "4294967296", "99999999999", "-1",
                                            "12a", " 5", "007", "4294967290", "429496729", "4294967300", "\x80"]]},
    {"name": "I_atoi64", "ret": "int64",
     "inputs": [[cstr(s)] for s in ["", "0", "-1", "9223372036854775807", "9223372036854775808", "-9223372036854775808",
                                    "  42xyz", "+17", "abc", "12345678901234"]]},
    {"name": "I_islower", "ret": "bool", "inputs": ints(SMALL_INTS)},
    {"name": "I_isupper", "ret": "bool", "inputs": ints(SMALL_INTS)},
    {"name": "I_isdigit", "ret": "bool", "inputs": ints(SMALL_INTS)},
    {"name": "I_isalnum", "ret": "bool", "inputs": ints(SMALL_INTS)},
    {"name": "I_isforfilename", "ret": "bool", "inputs": ints(SMALL_INTS)},
    {"name": "I_iscsym", "ret": "int", "inputs": ints(SMALL_INTS)},
    {"name": "I_strncpyz", "ret": "void",
     "inputs": [[b"\xcc" * 40, cstr(s), n] for s in STRINGS for n in (1, 2, 3, 5, 40)] +
               [[b"\xcc" * 40, 0, 10], [0, cstr("abc"), 10], [b"\xcc" * 40, cstr("abc"), 0]],
     "fakes": ASSERT_FAKE, "observe": ASSERT_SEEN},
    {"name": "I_strnicmp", "ret": "int",
     "inputs": [[cstr(a), cstr(b), n] for a, b in string_pairs() for n in (0, 2, 0x7FFFFFFF)] +
               [[0, cstr("a"), 5], [cstr("a"), 0, 5], [0, 0, 5]],
     "fakes": ASSERT_FAKE, "observe": ASSERT_SEEN},
    {"name": "I_strncmp", "ret": "int",
     "inputs": [[cstr(a), cstr(b), n] for a, b in string_pairs() for n in (0, 2, 0x7FFFFFFF)] +
               [[0, cstr("a"), 5], [cstr("a"), 0, 5], [0, 0, 5]],
     "fakes": ASSERT_FAKE, "observe": ASSERT_SEEN},
    {"name": "I_stristr", "ret": "cstr",
     "inputs": [[cstr(a), cstr(b)] for a, b in string_pairs()] + [[0, cstr("a")], [cstr("a"), 0]],
     "fakes": ASSERT_FAKE, "observe": ASSERT_SEEN},
    {"name": "I_stricmp", "ret": "int", "inputs": [[cstr(a), cstr(b)] for a, b in string_pairs()] + [[0, cstr("a")]],
     "fakes": ASSERT_FAKE, "observe": ASSERT_SEEN},
    {"name": "I_strcmp", "ret": "int", "inputs": [[cstr(a), cstr(b)] for a, b in string_pairs()] + [[0, cstr("a")]],
     "fakes": ASSERT_FAKE, "observe": ASSERT_SEEN},
    {"name": "I_stricmpwild", "ret": "int",
     "inputs": [[cstr(w), cstr(s)] for w in ["*", "", "a*", "*c", "a?c", "A*C", "*b*", "??", "?", "a*b*c", "*.ff",
                                             "mp_*", "**", "abc", "ab?", "\x80*", "[{*"]
                for s in ["", "abc", "ABC", "ab", "abd", "a", "mp_raid", "code.ff", "aXbYc", "\x80\x81"]] +
               [[0, cstr("a")], [cstr("a"), 0]],
     "fakes": ASSERT_FAKE, "observe": ASSERT_SEEN},
    {"name": "I_strlwr", "ret": "cstr", "inputs": [[cstr(s)] for s in STRINGS]},
    {"name": "I_strupr", "ret": "cstr", "inputs": [[cstr(s)] for s in STRINGS]},
    {"name": "I_strncat", "ret": "void",
     "inputs": [[cstr(d).ljust(48, b"\xcc"), size, cstr(s)] for d in ["", "abc", "x" * 10]
                for size in (4, 11, 12, 16, 48) for s in ["", "de", "longer string"]],
     "fakes": {**ASSERT_FAKE, "Com_Error": returns(None)}, "observe": {**ASSERT_SEEN, "Com_Error": "is"}},
    {"name": "I_DrawStrlen", "ret": "int",
     "inputs": [[cstr(s)] for s in STRINGS + ["^", "^^", "^^1", "^1", "^a", "a^1b^2c", "^9^0^/^:", "^1^"]]},
    {"name": "I_IsSpecialToken", "ret": "bool",
     "inputs": [[0]] + [[cstr(s)] for s in ["", "^", "^1", "^^", "[", "[{", "[x", "{", "a^1", "^\x80"]]},
    {"name": "I_IsSpecialTokenRecursive", "ret": "int",
     "inputs": [[0]] + [[cstr(s)] for s in ["", "^", "^1", "[{", "[[{", "^[{", "^^^", "[[", "[^", "[[[[x", "^[^["]]},
    {"name": "I_IsSpecialTokenString", "ret": "int",
     "inputs": [[0]] + [[cstr(s)] for s in ["", "^", "^1", "[{", "[[{", "^[{", "^^^", "[[", "[^"]]},
    {"name": "I_CleanChat", "ret": "cstr",
     "inputs": [[cstr(s)] for s in STRINGS + ["^^1x", "[{[{a}]", "a\tb\nc", "^", "~\x7f ", "^^^^^^", "[[{{"]] +
               [[cstr(random_text(rng.randrange(30)))] for _ in range(60)]},
    {"name": "I_CleanStr", "ret": "cstr",
     "inputs": [[cstr(s)] for s in STRINGS + ["^^1x", "^^^11a", "a\tb\nc", "^", "~\x7f ", "^1^2^3", "^a"]] +
               [[cstr(random_text(rng.randrange(30)))] for _ in range(60)]},
    {"name": "I_CleanChar", "ret": "char", "inputs": ints(range(-128, 128))},
    {"name": "Com_sprintf", "ret": "int",
     "inputs": [[bytes(32), n, cstr(f), *a] for n in (1, 4, 8, 32)
                for f, a in [("", []), ("abc", []), ("%s-%d", [cstr("name"), 42]), ("%08x|%5.2s|", [0xBEEF, cstr("xyz")]),
                             ("%%%c", [65]), ("%s", [cstr("x" * 40)])]]},
    {"name": "Com_sprintfPos", "ret": "int",
     "inputs": [[cstr(d).ljust(24, b"\xcc"), size, struct.pack("<i", pos), cstr(f), *a]
                for d, pos in [("", 0), ("abc", 3), ("abcdefgh", 8), ("x" * 15, 15)] for size in (8, 16, 24)
                for f, a in [("%s", [cstr("tail")]), ("%d", [-12345]), ("", [])]] +
               [[bytes(16), 16, struct.pack("<i", p), cstr("%s"), cstr("abc")] for p in (14, 15, 16, 40)]},
    {"name": "CanKeepStringPointer", "ret": "int",
     "inputs": [[p] for p in [0, 1, VA_INFO - 1, VA_INFO, VA_INFO + 0x800, VA_INFO + 0xFFF, VA_INFO + 0x1000,
                              SCRATCH, STACK, STACK + 0x1000, STACK + STACK_SIZE - 0x10, 0x7FFFFFFF, 0xFFFFFFFF]] +
               [[cstr("heap string")]],
     "fakes": {"Sys_GetValue": returns(VA_INFO)}, "observe": {"Sys_GetValue": "i"}},
    {"name": "I_itoa", "ret": "cstr",
     "inputs": [[v, bytes(16), 16] for v in [0, 1, -1, 9, 10, -10, 123456789, -123456789, 2**31 - 1, -2**31]] +
               [[rng.randrange(-2**31, 2**31), bytes(16), 12] for _ in range(80)] + [[5, bytes(16), 4]],
     "fakes": ASSERT_FAKE, "observe": ASSERT_SEEN},
    {"name": "va", "ret": "cstr",
     "inputs": [{"args": [cstr(f), *a], "setup": (lambda w, i=i: w.u32(VA_INFO + 0x1000, i))}
                for i in (0, 1, 3, -1, 7) for f, a in [("plain", []), ("%s=%d", [cstr("key"), 7]),
                                                       ("%s", [cstr("y" * 1100)]), ("%c%c", [ord("a"), ord("b")])]],
     "fakes": {"Sys_GetValue": returns(VA_INFO), "Com_Error": returns(None)},
     "observe": {"Com_Error": "iss", "Sys_GetValue": "i"}},
    {"name": "Com_InitThreadData", "ret": "void", "inputs": ints([0, 1, 2, 5, 16]),
     "fakes": {"Sys_SetValue": returns(None)}, "observe": {"Sys_SetValue": "iq"}},
    {"name": "Info_ValueForKey", "ret": "cstr", "inputs": info_value_inputs(),
     "fakes": {**ASSERT_FAKE, **THREAD_FAKES, "Com_Error": returns(None)},
     "observe": {**ASSERT_SEEN, "Com_Error": "isi"}},
    {"name": "Info_NextPair", "ret": "void",
     "inputs": [{"args": [struct.pack("<I", LAYOUT), b"\xcc" * 64, b"\xcc" * 64],
                 "setup": (lambda w, s=s: w.write(LAYOUT, cstr(s)))} for s in INFO + ["k\\v", "\\only"]]},
    {"name": "Info_RemoveKey", "ret": "void", "inputs": remove_inputs(1024) + [[cstr("x" * 1024), cstr("a")]],
     "fakes": {"Com_Error": returns(None)}, "observe": {"Com_Error": "is"}},
    {"name": "Info_RemoveKey_Big", "ret": "void", "inputs": remove_inputs(20480),
     "fakes": {"Com_Error": returns(None)}, "observe": {"Com_Error": "is"}},
    {"name": "Info_Validate", "ret": "int",
     "inputs": [[cstr(s)] for s in INFO + ['a"b', "a;b", '";', "plain text", ";", '"']]},
    {"name": "Info_SetValueForKey", "ret": "void", "inputs": set_inputs(1024), "fakes": SET_FAKES, "observe": SET_SEEN},
    {"name": "Info_SetValueForKey_Big", "ret": "void", "inputs": set_inputs(20480), "fakes": SET_FAKES,
     "observe": SET_SEEN},
    {"name": "ParseConfigStringToStruct", "ret": "int", "inputs": config_inputs(), "fakes": CONFIG_FAKES,
     "observe": CONFIG_SEEN},
    {"name": "ParseConfigStringToStructMerged", "ret": "int", "inputs": merged_inputs(), "fakes": CONFIG_FAKES,
     "observe": CONFIG_SEEN},
    {"name": "GetLeanFraction", "ret": "float",
     "inputs": [[("f", x)] for x in floats() + [rng.uniform(-1, 1) for _ in range(200)]]},
    {"name": "UnGetLeanFraction", "ret": "float",
     "inputs": [[("f", x)] for x in [0.0, 1.0, 0.8, 0.80000001, 0.9, 0.5, 1e-30, 0.99999994, -0.5, 1.5,
                                     float("nan"), float("inf")] + [rng.uniform(0, 1) for _ in range(200)]],
     "fakes": ASSERT_FAKE, "observe": ASSERT_SEEN},
    {"name": "AddLeanToPosition", "ret": "void",
     "inputs": [[vec(*rand_floats(3, 1000)), ("f", rng.uniform(-360, 360)), ("f", lean), ("f", rng.uniform(-30, 30)),
                 ("f", rng.uniform(0, 64))] for lean in [0.0, -0.0, 1.0, -1.0, 0.5, float("nan")] + rand_floats(60, 1.2)]},
    {"name": "OrientationConcatenate", "ret": "void",
     "inputs": [[orientation(), orientation(), bytes(48)] for _ in range(100)] +
               [[vec(*([float("nan")] * 12)), orientation(), bytes(48)]],
     "fakes": ASSERT_FAKE, "observe": ASSERT_SEEN},
    {"name": "OrientationInvert", "ret": "void",
     "inputs": [[orientation(), bytes(48)] for _ in range(100)] + [[vec(*([-0.0] * 12)), bytes(48)]]},
    {"name": "Com_IsLegacyXModelName", "ret": "int",
     "inputs": [[cstr(s)] for s in ["xmodel/a", "XMODEL\\b", "xmodel", "xmodelx", "xmode", "", "models/xmodel/a",
                                    "xModel/"]]},
    {"name": "CMD_GetAnalogButtonValue", "ret": "float",
     "inputs": [[usercmd(b), bit] for b in [0, 1 << 63, (1 << 64) - 1, 0x0000_0000_0000_1C00 << 32,
                                            rng.getrandbits(64), rng.getrandbits(64)]
                for bit in list(range(0, 64)) + [64, 100, -1]],
     "fakes": {"Assert_MyHandler": returns(1)}, "observe": {"Assert_MyHandler": "siisss"}},
]
