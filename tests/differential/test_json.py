"""json/json.cpp on the server (the Store build: tests/replay/verify_json.py)."""
import random
import struct

from harness import OBJECTS

SERVER = ["CoDMPServer_PC"]
# analysis/CoDMPServer_PC/pdb/statics.tsv
S_JSON, S_CURR, S_NUM, S_TOKENS, S_PARSER, S_VAL = 0x01CF7C84, 0x01CF7C88, 0x01CF7C8C, 0x01CF7C90, 0x01CF7C94, 0x01CF7880
TEXT, TOKENS = OBJECTS + 0x1000, OBJECTS + 0x4000
rng = random.Random(0x68C5D0)

SAMPLES = [
    b'{"version":3,"motdVersion":12,"singleView":true,"expiration":1700000000,"imageCategory":4}',
    b'{"data":[{"vis":"IsElite","motd":{"title":"Hi","message":"a\\"b\\\\c\\n"}},{"vis":""}],"elite":{}}',
    b'[1, -2, 3.5, true, false, null, "x"]', b'{"a":{"b":[[],{}]}}', b'{}', b'[]', b'', b'   \t\r\n',
    b'"lone string"', b'42', b'{"a":1', b'{"a":"unterminated', b'[1,2]]', b'}', b'{"a":[1}', b'{"bad\\q":1}',
    b'{"ctl":\x01}', b'{"hi":\x7f}', b'{"\xc3\xa9t\xc3\xa9":"caf\xc3\xa9"}', b'{"ref":"MENU_X","noloc":"y"}',
    b'{"a" : 1 , "b" : [ "c" , { "d" : false } ] }', b'::::', b'{"k":"v"}:', b'[,,,]', b'tru', b'{"a":1}{"b":2}',
    b'{"long":"' + b"x" * 1500 + b'"}', b'[' + b",".join(b"%d" % i for i in range(40)) + b']',
]


def parser(pos=0, toknext=0, toksuper=-1):
    return struct.pack("<Iii", pos, toknext, toksuper)


def tokens(n, filled=0):
    """n tokens of junk, the first `filled` of them in use (start and end not -1)."""
    out = b""
    for i in range(n):
        start = rng.randrange(0, 50) if i < filled else rng.choice([-1, 5])
        out += struct.pack("<iiiiII", rng.randrange(5), start, rng.randrange(0, 60), rng.randrange(9),
                           rng.getrandbits(32), rng.getrandbits(32))
    return out


def parse_inputs(with_pos):
    out = []
    for js in SAMPLES:
        for n in (64, 3, 1, 0):
            for toknext in (0, 1) if n else (0,):
                p = parser(0, toknext, -1)
                args = [p, js + b"\0", tokens(n, toknext), n]
                out.append({"args": args + [bytes(4)] if with_pos else args})
    return out


def py_tokens(js, n=128):
    """A token array as jsmn_parse would fill it (computed by the reconstruction's rules), for the reader."""
    toks, stack, i = [], [], 0
    while i < len(js):
        c = js[i:i + 1]
        if c in b"{[":
            if stack:
                toks[stack[-1]][3] += 1
            toks.append([1 if c == b"{" else 2, i, -1, 0])
            stack.append(len(toks) - 1)
        elif c in b"}]":
            toks[stack.pop()][2] = i + 1
        elif c == b'"':
            j = i + 1
            while js[j:j + 1] != b'"':
                j += 2 if js[j:j + 1] == b"\\" else 1
            if stack:
                toks[stack[-1]][3] += 1
            toks.append([4, i + 1, j, 0])
            i = j
        elif c == b":":
            toks[-1][0] = 3
        elif c not in b" \t\r\n,":
            j = i
            while j < len(js) and js[j:j + 1] not in b" \t\r\n,:]}":
                j += 1
            if stack:
                toks[stack[-1]][3] += 1
            toks.append([0, i, j, 0])
            i = j - 1
        i += 1
    return toks


READER = [b'{"version":3,"title":"Hello","flag":true,"off":false,"n":-17,"z":null,"o":{},"a":[]}',
          b'{"loc":{"english":"Hi","french":"Salut"}}', b'{"loc":{"noloc":"same"}}',
          b'{"loc":{"ref":"MENU_REF"}}', b'{"loc":{"german":"Hallo"}}', b'{"loc":{"english":1}}',
          b'{"loc":"notobject"}', b'{"loc":{"a":"b","c":"d","english":"late"}}',
          b'{"big":"' + b"y" * 1200 + b'"}', b'{"loc":{"ENGLISH":"case"}}', b'{"loc":{}}']


def session(js, curr=0, num=None, json_ptr=True, tok_ptr=True):
    toks = py_tokens(js)
    blob = b"".join(struct.pack("<iiiiII", t, s, e, z, 0xAAAAAAAA, 0xBBBBBBBB) for t, s, e, z in toks)

    def run(w):
        w.write(TEXT, js + b"\0")
        w.write(TOKENS, blob)
        w.write(S_JSON, struct.pack("<I", TEXT if json_ptr else 0))
        w.write(S_TOKENS, struct.pack("<I", TOKENS if tok_ptr else 0))
        w.write(S_CURR, struct.pack("<i", curr))
        w.write(S_NUM, struct.pack("<i", len(toks) if num is None else num))
    return run, len(toks)


def reader_inputs(arg=None):
    out = []
    for js in READER:
        _, n = session(js)
        for curr in range(0, n + 1):
            out.append({"args": [] if arg is None else [arg(curr)], "setup": session(js, curr)[0]})
    run, _ = session(READER[0], 0, json_ptr=False)
    out.append({"args": [] if arg is None else [arg(0)], "setup": run})
    run, _ = session(READER[0], 0, tok_ptr=False)
    out.append({"args": [] if arg is None else [arg(0)], "setup": run})
    return out


def translate(mu):
    return OBJECTS + 0x100


FAKES = {"UI_SafeTranslateString": translate, "Com_PrintError": lambda mu: 0, "Com_PrintWarning": lambda mu: 0,
         "Assert_MyHandler": lambda mu: 1}
OBSERVE = {"UI_SafeTranslateString": "s", "Com_PrintError": "is", "Com_PrintWarning": "is"}
LOCS = [b"english\0", b"french\0", b"\0", 0]

CASES = [
    {"name": "jsmn_init", "ret": "void", "builds": SERVER, "inputs": [{"args": [bytes(12)]}]},
    {"name": "?jsmn_parse@@YA?AW4jsmnerr_t@@PAUjsmn_parser@@PBDPAUjsmntok_t@@IAAH@Z", "ret": "int", "builds": SERVER,
     "inputs": parse_inputs(True)},
    {"name": "?jsmn_parse@@YA?AW4jsmnerr_t@@PAUjsmn_parser@@PBDPAUjsmntok_t@@I@Z", "ret": "int", "builds": SERVER,
     "inputs": parse_inputs(False)},
    {"name": "JSMN_BeginParseSession", "ret": "bool", "builds": SERVER, "fakes": FAKES, "observe": OBSERVE,
     "inputs": [{"args": [js + b"\0", TOKENS, n]} for js in SAMPLES for n in (128, 2)]},
    {"name": "JSMN_GetToken", "ret": "uint", "builds": SERVER, "fakes": FAKES, "observe": OBSERVE,
     "inputs": reader_inputs()},
    {"name": "JSMN_UnGetToken", "ret": "void", "builds": SERVER,
     "inputs": [{"args": [], "setup": session(READER[0], c)[0]} for c in (0, 1, 5)]},
    {"name": "JSMN_GetKey", "ret": "uint", "builds": SERVER, "fakes": FAKES, "observe": OBSERVE,
     "inputs": reader_inputs(lambda c: [b"version\0", b"TITLE\0", b"loc\0", b"\0"][c % 4])},
    {"name": "JSMN_GetLocalizedString", "ret": "cstr", "builds": SERVER, "fakes": FAKES, "observe": OBSERVE,
     "inputs": reader_inputs(lambda c: LOCS[c % 4])},
]

# jsmn_parse's two helpers, called from where jsmn_parse is (the parser at a character of that kind)
def helper_inputs(first):
    out = []
    for js in SAMPLES + [b'"a\\u12"', b'"\\x"', b'abc', b'-1}', b'"open', b'tr\x05e', b'"' + b"z" * 50 + b'"']:
        for pos in range(len(js)):
            if (js[pos:pos + 1] == b'"') == first:
                for n in (8, 1, 0):
                    out.append({"args": [parser(pos, 0, -1), js + b"\0", tokens(8, 0), n]})
    return out[::3]


CASES += [
    {"name": "jsmn_parse_primitive", "ret": "int", "builds": SERVER, "inputs": helper_inputs(False)},
    {"name": "jsmn_parse_string", "ret": "int", "builds": SERVER, "inputs": helper_inputs(True)},
]
