"""Compare reconstructed leaf functions with the pinned PC server's x86 instructions."""
import argparse
import ctypes
import csv
import hashlib
import random
import struct
import tomllib
from pathlib import Path

import pefile
from unicorn import UC_ARCH_X86, UC_MODE_32, Uc
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP

ROOT = Path(__file__).resolve().parents[2]
CODE = 0x1000
RETURN = 0x2000
STACK = 0x8000
DATA = 0xA000
DATA_SIZE = 0x20000
FUNCTIONS = {
    "ClampChar": (0x00761340, 24, ctypes.c_int8, [ctypes.c_int32]),
    "ClampShort": (0x00761360, 34, ctypes.c_int16, [ctypes.c_int32]),
    "Com_HashKey": (0x0076B690, 73, ctypes.c_int32, [ctypes.c_char_p, ctypes.c_int32]),
}


def original_function(image, address, size):
    vm = Uc(UC_ARCH_X86, UC_MODE_32)
    vm.mem_map(CODE, 0x2000)
    vm.mem_map(STACK, 0x1000)
    vm.mem_map(DATA, DATA_SIZE)
    code = image.get_data(address - image.OPTIONAL_HEADER.ImageBase, size)
    if len(code) != size:
        raise ValueError(f"Incomplete original code at {address:08X}")
    vm.mem_write(CODE, code)

    def invoke(arguments, data=None):
        if data is not None:
            vm.mem_write(DATA, data + b"\0")
        sp = STACK + 0x800
        stack_words = (RETURN, *(arg & 0xffffffff for arg in arguments))
        vm.mem_write(sp, struct.pack(f"<{len(stack_words)}I", *stack_words))
        vm.reg_write(UC_X86_REG_ESP, sp)
        vm.emu_start(CODE, RETURN, count=2_000_000)
        if vm.reg_read(UC_X86_REG_EIP) != RETURN:
            raise RuntimeError(f"Original function at {address:08X} did not return")
        return vm.reg_read(UC_X86_REG_EAX)

    return invoke


def cases(name):
    rng = random.Random(6)
    if name == "Com_HashKey":
        yield (None, 0)
        yield (None, 50)
        for data in (b"", b"a", b"Black Ops II", b"ab\0cd", bytes(range(1, 256)), b"\x7f" * 100_000):
            for limit in (-1, 0, 1, 4, len(data), len(data) + 1):
                yield (data, limit)
        for _ in range(250):
            data = rng.randbytes(rng.randrange(512))
            yield (data, rng.randrange(-10, len(data) + 10))
    else:
        for value in (-2**31, -32769, -32768, -32767, -129, -128, -127, 0, 126, 127, 128, 32766, 32767, 32768, 2**31 - 1):
            yield (value,)
        for _ in range(250):
            yield (rng.randrange(-2**31, 2**31),)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original", type=Path)
    parser.add_argument("library", type=Path)
    args = parser.parse_args()
    config = tomllib.loads((ROOT / "target.toml").read_text())
    server = next(build for build in config["build"] if build["id"] == "CoDMPServer_PC")
    binary = args.original.read_bytes()
    if hashlib.sha256(binary).hexdigest() != server["exe"]["sha256"]:
        raise ValueError("Original PC server SHA-256 mismatch")
    with (ROOT / "records" / "functions.tsv").open(newline="") as records:
        verified = {
            (row["build"], int(row["va"], 16))
            for row in csv.DictReader(records, delimiter="\t")
            if row["status"] in {"matches", "integrated"}
        }
    covered = {(server["id"], address) for address, *_ in FUNCTIONS.values()}
    if verified != covered:
        raise ValueError(f"Function records and differential tests disagree: {verified ^ covered}")
    image = pefile.PE(data=binary, fast_load=True)
    library = ctypes.CDLL(str(args.library.resolve()))
    for name, (address, size, result_type, arg_types) in FUNCTIONS.items():
        original = original_function(image, address, size)
        reconstructed = getattr(library, name)
        reconstructed.restype = result_type
        reconstructed.argtypes = arg_types
        count = 0
        for arguments in cases(name):
            if name == "Com_HashKey":
                data, limit = arguments
                result = original((DATA if data is not None else 0, limit), data)
            else:
                result = original(arguments)
            expected = result_type(result).value
            actual = reconstructed(*arguments)
            count += 1
            if actual != expected:
                raise AssertionError(f"{name} case {count}: original {expected}, reconstructed {actual}")
        print(f"{name}: {count} cases passed")


if __name__ == "__main__":
    main()
