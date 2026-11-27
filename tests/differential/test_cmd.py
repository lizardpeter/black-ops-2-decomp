"""qcommon/cmd.cpp's command registration on the server (the Store build inlines it)."""
import struct

from harness import OBJECTS

SERVER = ["CoDMPServer_PC"]
CMD_FUNCTIONS = 0x01D4F494  # analysis/CoDMPServer_PC/pdb/statics.tsv
NAMES = OBJECTS + 0x800
RECORDS = OBJECTS + 0x1000  # 0x14 bytes each
NEW = OBJECTS + 0x1800


def chain(names):
    """cmd_functions: one record per name, in order."""
    def run(w):
        text = b""
        offsets = []
        for n in names:
            offsets.append(len(text))
            text += n + b"\0"
        w.write(NAMES, text)
        for i, off in enumerate(offsets):
            nxt = RECORDS + 0x14 * (i + 1) if i + 1 < len(names) else 0
            w.write(RECORDS + 0x14 * i, struct.pack("<5I", nxt, NAMES + off, 0, 0, 0x00401000 + i))
        w.write(NEW, bytes(0x14))
        w.write(CMD_FUNCTIONS, struct.pack("<I", RECORDS if names else 0))
    return run


LISTS = [[], [b"exec"], [b"exec", b"set", b"motdGetImageDescriptors"], [b"Exec", b"exe", b"execx"]]
QUERIES = [b"exec", b"EXEC", b"set", b"motdGetImageDescriptors", b"", b"zz"]
FAKES = {"Com_Printf": lambda mu: 0, "Assert_MyHandler": lambda mu: 1}
OBSERVE = {"Com_Printf": "is", "Assert_MyHandler": "sii"}

CASES = [
    {"name": "Cmd_FindCommand", "ret": "uint", "builds": SERVER,
     "inputs": [{"args": [q + b"\0"], "setup": chain(l)} for l in LISTS for q in QUERIES]},
    {"name": "Cmd_AddCommandInternal", "ret": "void", "builds": SERVER, "fakes": FAKES, "observe": OBSERVE,
     "inputs": [{"args": [q + b"\0", f, rec], "setup": chain(l)} for l in LISTS for q in QUERIES
                for f in (0, 0x00402000) for rec in (NEW, RECORDS)]},
]
