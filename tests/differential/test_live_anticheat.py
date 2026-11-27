"""live/live_anticheat.cpp's hashing on the server (the Store build: tests/replay/verify_live_anticheat.py)."""
import random
import struct

from harness import OBJECTS

SERVER = ["CoDMPServer_PC"]
S_HASH_INFOS = 0x07F1DC10  # analysis/CoDMPServer_PC/pdb/statics.tsv
DATA = OBJECTS + 0x1000
rng = random.Random(0x8CA570)


def info(type_, start=0, length=0, position=0, block=0x2000, state=0):
    return struct.pack("<5IxxxxQ", type_, start, length, position, block, state)


def pool(slots, data=b""):
    """s_hashInfos: (index, packed HashInfo) pairs, the rest free; and the bytes being hashed at DATA."""
    def run(w):
        table = bytearray(16 * 0x20)
        for i, packed in slots:
            table[i * 0x20:(i + 1) * 0x20] = packed
        w.write(S_HASH_INFOS, bytes(table))
        if data:
            w.write(DATA, data)
    return run


def blob(n):
    return bytes(rng.randrange(256) for _ in range(n))


def start_inputs():
    out = []
    for used in (0, 1, 5, 15, 16):
        for type_ in (0, 1, 2, 3, 4):
            busy = [(i, info(1, 1, 2, 0, 0x2000, 9)) for i in range(used)]
            out.append({"args": [type_, rng.getrandbits(32), rng.getrandbits(16), 0x2000, ("q", rng.getrandbits(64))],
                        "setup": pool(busy)})
    return out


def continue_inputs():
    out = []
    for type_ in (1, 2, 3):
        for length, position, block in ((0, 0, 0x2000), (100, 0, 0x2000), (100, 40, 16), (0x3000, 0, 0x2000),
                                        (0x3000, 0x2000, 0x2000), (33, 33, 8), (7, 0, 1)):
            data = blob(length)
            slot = rng.randrange(16)
            h = info(type_, DATA + rng.choice([0, 1]), length, position, block, rng.getrandbits(64))
            out.append({"args": [S_HASH_INFOS + 0x20 * slot], "setup": pool([(slot, h)], data + b"\0")})
    return out


def complete_inputs():
    out = []
    for type_ in (1, 2, 3):
        slot = rng.randrange(16)
        out.append({"args": [S_HASH_INFOS + 0x20 * slot, bytes(8)],
                    "setup": pool([(slot, info(type_, 5, 6, 6, 0x2000, rng.getrandbits(64)))])})
    return out


CASES = [
    {"name": "LiveAntiCheat_GenerateCRC32Checksum", "ret": "void", "builds": SERVER,
     "inputs": [[blob(n) + b"\0", n, struct.pack("<I", rng.getrandbits(32))] for n in (0, 1, 2, 9, 64, 255)]},
    {"name": "LiveAntiCheat_GenerateCRC32ChecksumSplit", "ret": "uint64", "builds": SERVER,
     "inputs": [[blob(n) + b"\0", n, ("q", rng.getrandbits(64))] for n in (0, 1, 2, 9, 64, 255)]},
    {"name": "LiveAntiCheat_StartHash", "ret": "uint", "builds": SERVER, "inputs": start_inputs(),
     "fakes": {"Assert_MyHandler": lambda mu: 1}},
    {"name": "LiveAntiCheat_ContinueHash", "ret": "bool", "builds": SERVER, "inputs": continue_inputs(),
     "fakes": {"Assert_MyHandler": lambda mu: 1}},
    {"name": "LiveAntiCheat_CompleteHash", "ret": "bool", "builds": SERVER, "inputs": complete_inputs()},
]
