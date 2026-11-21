"""universal/: q_shared.cpp, com_math.cpp, com_shared.cpp."""
import random

rng = random.Random(6)


def ints32(extra=()):
    vals = [-2**31, -32769, -32768, -32767, -129, -128, -127, -1, 0, 1, 126, 127, 128, 32766, 32767, 32768, 2**31 - 1]
    return [[v] for v in [*vals, *extra, *(rng.randrange(-2**31, 2**31) for _ in range(250))]]


def bits(width):
    vals = [0, 1, (1 << width) - 1, 0x0123456789ABCDEF & ((1 << width) - 1)]
    vals += [1 << b for b in range(width)] + [rng.getrandbits(width) for _ in range(250)]
    return vals


def hash_inputs():
    out = [[0, 0], [0, 50]]  # a null string
    for data in (b"", b"a", b"Black Ops II", b"ab\0cd", bytes(range(1, 256)), b"\x7f" * 100_000):
        for limit in (-1, 0, 1, 4, len(data), len(data) + 1):
            out.append([data + b"\0", limit])
    for _ in range(250):
        data = rng.randbytes(rng.randrange(512))
        out.append([data + b"\0", rng.randrange(-10, len(data) + 10)])
    return out


CASES = [
    {"name": "ClampChar", "ret": "char", "inputs": ints32()},
    {"name": "ClampShort", "ret": "short", "inputs": ints32()},
    {"name": "Com_HashKey", "ret": "int", "inputs": hash_inputs()},
    {"name": "ColorIndex", "ret": "uchar", "inputs": [[v] for v in range(256)]},
    {"name": "ShortSwap", "ret": "short", "inputs": [[v] for v in range(-2**15, 2**15, 7)] + [[-1], [2**15 - 1]]},
    {"name": "ShortNoSwap", "ret": "short", "inputs": [[v] for v in range(-2**15, 2**15, 7)]},
    {"name": "LongSwap", "ret": "int", "inputs": [[v - 2**32 if v >> 31 else v] for v in bits(32)]},
    {"name": "Long64Swap", "ret": "uint64", "inputs": [[("q", v)] for v in bits(64)]},
    {"name": "Long64NoSwap", "ret": "uint64", "inputs": [[("q", v)] for v in bits(64)]},
    {"name": "I_islower", "ret": "bool", "inputs": [[v] for v in range(-256, 257)] + ints32()},
    {"name": "I_isdigit", "ret": "bool", "inputs": [[v] for v in range(-256, 257)] + ints32()},
]
