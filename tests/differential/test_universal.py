"""universal/: com_math.cpp, com_shared.cpp (q_shared.cpp: test_q_shared.py)."""
import random

rng = random.Random(6)


def ints32(extra=()):
    vals = [-2**31, -32769, -32768, -32767, -129, -128, -127, -1, 0, 1, 126, 127, 128, 32766, 32767, 32768, 2**31 - 1]
    return [[v] for v in [*vals, *extra, *(rng.randrange(-2**31, 2**31) for _ in range(250))]]



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
]
