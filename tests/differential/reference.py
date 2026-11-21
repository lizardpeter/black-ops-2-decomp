"""Compare reconstructed functions with the pinned original builds' code, x86 or x64, under Unicorn.

    python tests/differential/reference.py <original.exe> <library> [--build <build-id>]

The whole original image is mapped at its preferred base, so a routine runs exactly as in the game, reading its own
constants and tables. x86 builds use cdecl (stack arguments, result in edx:eax); x64 builds the Windows x64
convention (rcx, rdx, r8, r9; result in rax).

Where each function sits: FUNCTIONS gives the PDB-backed CoDMPServer_PC address. Every other build takes its
addresses from records/names.tsv (name -> va, with the evidence for the identification). The set tested must equal the
set records/functions.tsv marks `matches` or better for that build, so a status can never claim more than this test
shows.
"""
import argparse
import csv
import ctypes
import hashlib
import random
import struct
import tomllib
from pathlib import Path

import pefile
from unicorn import UC_ARCH_X86, UC_MODE_32, UC_MODE_64, Uc, UcError
from unicorn import x86_const as R

ROOT = Path(__file__).resolve().parents[2]
REFERENCE = "CoDMPServer_PC"
# name: (CoDMPServer_PC va, size there, result type, argument types)
FUNCTIONS = {
    "ClampChar": (0x00761340, 24, ctypes.c_int8, [ctypes.c_int32]),
    "ClampShort": (0x00761360, 34, ctypes.c_int16, [ctypes.c_int32]),
    "Com_HashKey": (0x0076B690, 73, ctypes.c_int32, [ctypes.c_char_p, ctypes.c_int32]),
    "ShortSwap": (0x0078DCC0, 24, ctypes.c_int16, [ctypes.c_int16]),
    "ShortNoSwap": (0x0078DCE0, 9, ctypes.c_int16, [ctypes.c_int16]),
    "LongSwap": (0x0078DCF0, 48, ctypes.c_int32, [ctypes.c_int32]),
    "Long64Swap": (0x0078DD20, 173, ctypes.c_uint64, [ctypes.c_uint64]),
    "Long64NoSwap": (0x0078DDD0, 11, ctypes.c_uint64, [ctypes.c_uint64]),
    "ColorIndex": (0x0078DB60, 22, ctypes.c_uint8, [ctypes.c_uint8]),
    "I_islower": (0x0078DF00, 21, ctypes.c_uint8, [ctypes.c_int32]),
    "I_isdigit": (0x0078DF40, 21, ctypes.c_uint8, [ctypes.c_int32]),
}
POINTER_ARGS = {"Com_HashKey": 0}  # argument index that is a pointer to the case's data buffer


class Emulator:
    """One Unicorn machine with the whole original image mapped at its preferred base."""
    STACK, STACK_SIZE = 0x00010000, 0x00040000
    DATA, DATA_SIZE = 0x00100000, 0x00040000
    RETURN = 0x00001000  # mapped, never executed: emulation stops on reaching it

    def __init__(self, pe):
        self.x64 = pe.FILE_HEADER.Machine == 0x8664
        self.base = pe.OPTIONAL_HEADER.ImageBase
        self.vm = Uc(UC_ARCH_X86, UC_MODE_64 if self.x64 else UC_MODE_32)
        size = (pe.OPTIONAL_HEADER.SizeOfImage + 0xFFF) & ~0xFFF
        self.vm.mem_map(self.base, size)
        self.vm.mem_write(self.base, pe.header[:pe.OPTIONAL_HEADER.SizeOfHeaders])
        for s in pe.sections:
            data = s.get_data()[:max(s.Misc_VirtualSize, 0) or len(s.get_data())]
            self.vm.mem_write(self.base + s.VirtualAddress, data)
        self.vm.mem_map(self.RETURN, 0x1000)
        self.vm.mem_map(self.STACK, self.STACK_SIZE)
        self.vm.mem_map(self.DATA, self.DATA_SIZE)
        self.pe = pe
        self._pdata = None

    def call(self, va, name, arguments, restype, max_insns=2_000_000):
        """Run the original routine at va on one case; the result as restype, or None if it faulted or ran away."""
        args = list(arguments)
        ptr = POINTER_ARGS.get(name)
        if ptr is not None:
            data = args[ptr]
            if data is not None:
                if len(data) + 1 > self.DATA_SIZE:
                    raise ValueError("case data larger than the emulator's data buffer")
                self.vm.mem_write(self.DATA, data + b"\0")
            args[ptr] = self.DATA if data is not None else 0
        sp = self.STACK + self.STACK_SIZE - 0x1000
        try:
            if self.x64:
                regs = (R.UC_X86_REG_RCX, R.UC_X86_REG_RDX, R.UC_X86_REG_R8, R.UC_X86_REG_R9)
                for reg, value in zip(regs, args):
                    self.vm.reg_write(reg, value & 0xFFFFFFFFFFFFFFFF)
                sp -= 0x28  # return address + 32-byte home space
                self.vm.mem_write(sp, struct.pack("<Q", self.RETURN) + bytes(0x20))
                self.vm.reg_write(R.UC_X86_REG_RSP, sp)
                self.vm.emu_start(va, self.RETURN, count=max_insns)
                if self.vm.reg_read(R.UC_X86_REG_RIP) != self.RETURN:
                    return None
                result = self.vm.reg_read(R.UC_X86_REG_RAX)
            else:
                words = []
                for value, typ in zip(args, FUNCTIONS[name][3]):
                    if typ is ctypes.c_uint64:
                        words += [value & 0xFFFFFFFF, (value >> 32) & 0xFFFFFFFF]
                    else:
                        words.append(value & 0xFFFFFFFF)
                stack = struct.pack(f"<{len(words) + 1}I", self.RETURN, *words)
                sp -= len(stack)
                self.vm.mem_write(sp, stack)
                self.vm.reg_write(R.UC_X86_REG_ESP, sp)
                self.vm.emu_start(va, self.RETURN, count=max_insns)
                if self.vm.reg_read(R.UC_X86_REG_EIP) != self.RETURN:
                    return None
                result = (self.vm.reg_read(R.UC_X86_REG_EDX) << 32) | self.vm.reg_read(R.UC_X86_REG_EAX)
        except UcError:
            return None
        return restype(result).value

    def extent(self, va):
        """Bytes from va to the end of its .pdata range (x64), else to the first ret followed by padding."""
        if self.x64:
            if self._pdata is None:
                sec = next(s for s in self.pe.sections if s.Name.rstrip(b"\0") == b".pdata")
                d = sec.get_data()[:sec.Misc_VirtualSize]
                self._pdata = {self.base + struct.unpack_from("<I", d, i)[0]:
                               struct.unpack_from("<I", d, i + 4)[0] - struct.unpack_from("<I", d, i)[0]
                               for i in range(0, len(d) - 11, 12)}
            if va in self._pdata:
                return self._pdata[va]
        code = bytes(self.vm.mem_read(va, 0x1000))
        for i, b in enumerate(code[:-1]):
            if b == 0xC3 and code[i + 1] in (0xCC, 0x90):
                return i + 1
        return 0


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
    elif name == "ColorIndex":
        for value in range(256):
            yield (value,)
    elif name in {"I_islower", "I_isdigit"}:
        for value in range(-256, 257):
            yield (value,)
        for value in (-2**31, 2**31 - 1):
            yield (value,)
        for _ in range(250):
            yield (rng.randrange(-2**31, 2**31),)
    elif name in {"ShortSwap", "ShortNoSwap"}:
        for value in range(-2**15, 2**15):
            yield (value,)
    elif name in {"LongSwap", "Long64Swap", "Long64NoSwap"}:
        width = 32 if name == "LongSwap" else 64
        value_type = ctypes.c_int32 if width == 32 else ctypes.c_uint64
        for value in (0, 1, (1 << width) - 1, 0x0123456789abcdef):
            yield (value_type(value).value,)
        for bit in range(width):
            yield (value_type(1 << bit).value,)
        for _ in range(250):
            yield (value_type(rng.getrandbits(width)).value,)
    else:
        for value in (-2**31, -32769, -32768, -32767, -129, -128, -127, 0, 126, 127, 128, 32766, 32767, 32768, 2**31 - 1):
            yield (value,)
        for _ in range(250):
            yield (rng.randrange(-2**31, 2**31),)


def call_reconstructed(fn, name, arguments):
    return fn(*arguments)


def read_tsv(path):
    with open(path, newline="") as f:
        return list(csv.DictReader(f, delimiter="\t", quoting=csv.QUOTE_NONE))


def locations(build_id):
    """name -> va of the functions this build is tested on."""
    if build_id == REFERENCE:
        return {name: spec[0] for name, spec in FUNCTIONS.items()}
    return {r["name"]: int(r["va"], 16) for r in read_tsv(ROOT / "records" / "names.tsv")
            if r["build"] == build_id and r["kind"] == "function" and r["name"] in FUNCTIONS}


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("original", type=Path)
    parser.add_argument("library", type=Path)
    parser.add_argument("--build", default=REFERENCE)
    args = parser.parse_args()
    config = tomllib.loads((ROOT / "target.toml").read_text())
    build = next(b for b in config["build"] if b["id"] == args.build)
    binary = args.original.read_bytes()
    if hashlib.sha256(binary).hexdigest() != build["exe"]["sha256"]:
        raise ValueError(f"Original {args.build} SHA-256 mismatch")
    verified = {int(r["va"], 16) for r in read_tsv(ROOT / "records" / "functions.tsv")
                if r["build"] == args.build and r["status"] in {"matches", "integrated"}}
    where = locations(args.build)
    if verified != set(where.values()):
        raise ValueError(f"{args.build}: records/functions.tsv and the tested set disagree at "
                         + ", ".join(f"{va:08X}" for va in sorted(verified ^ set(where.values()))))
    emu = Emulator(pefile.PE(data=binary))
    library = ctypes.CDLL(str(args.library.resolve()))
    for name, va in sorted(where.items(), key=lambda kv: kv[1]):
        _, _, restype, argtypes = FUNCTIONS[name]
        fn = getattr(library, name)
        fn.restype, fn.argtypes = restype, argtypes
        count = 0
        for arguments in cases(name):
            expected = emu.call(va, name, arguments, restype)
            actual = call_reconstructed(fn, name, arguments)
            count += 1
            if expected is None:
                raise AssertionError(f"{name} at {va:08X} case {count}: the original faulted or did not return")
            if actual != expected:
                raise AssertionError(f"{name} at {va:08X} case {count}: original {expected}, reconstructed {actual}")
        print(f"{args.build} {name} {va:08X}: {count} cases passed")


if __name__ == "__main__":
    main()
