"""Run original and reconstructed functions side by side in one emulator, x86 or x64.

An original exe (verified by sha256 against target.toml) and its build/<build>/recon.dll are both mapped into one
Unicorn address space of the exe's architecture. A call follows the build's calling convention, runs to a sentinel
return address, then reads the result and every buffer passed by pointer. Comparing the two runs on the same
inputs checks behaviour, independent of which compiler built the reconstruction. Merged from black-ops-1-decomp
(x86) and black-ops-3-decomp (x64).

Arguments: ints, ('q', v) for a 64-bit integer, ('f', x) float, ('d', x) double, or bytes (passed by pointer, read
back after the call).
- x86 (cdecl): pushed right to left, one dword each (two for 'q' and 'd'). Results in EAX, EDX:EAX (int64) or ST0
  (float, double: read from the x87 stack unrounded, as the caller would see it). FS selects a GDT descriptor on a
  TEB (32-bit mode ignores FS_BASE): self pointer at 0x18, TLS array at 0x2C. x87 control word 0x27F and MXCSR
  0x1F80, as Windows starts a thread.
- x64 (Win64): position i < 4 in RCX/RDX/R8/R9 or XMM0..XMM3, the rest on the stack above 32 bytes of shadow space;
  RSP % 16 == 8 at entry. Results in RAX or XMM0. GS on a TEB (PEB with the exe's base, TLS array at 0x58).

Imports: every import address table slot of the exe points at a stub. On x64 an unhooked import returns 0. On x86
it faults instead, naming the import: Win32 functions are stdcall and pop their own arguments, which a generic stub
can't know, so a test that reaches one registers it with Machine.on_import(name, fn, nargs).

rdtsc is deterministic. Writes into either image or the TLS block are journaled: each call reports them, then they
are rolled back, so calls never see each other's state. `fakes` replaces an original function with Python
(fn(mu) -> return value) for both runs alike; `observe` records the arguments of every call to an address; `at`
runs Python at an address without leaving the function.
"""
import hashlib
import math
import os
import struct
import sys
import tomllib
from pathlib import Path

import pefile
from unicorn import UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_INSN, UC_HOOK_MEM_WRITE, UC_MODE_32, UC_MODE_64, Uc, UcError
from unicorn import x86_const as X
from unicorn.unicorn_py3.arch.intel import HOOK_INSN_CPUID_CFUNC  # rdtsc hooks share the cpuid callback type
from unicorn.unicorn_py3.unicorn import uccallback

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import symbols  # noqa: E402

STACK, STACK_SIZE = 0x00100000, 0x00100000
STACK_FILL = 0x10000  # the top of the stack, refilled before each call
SENTINEL = 0x00200000
HEAP, HEAP_SIZE = 0x00300000, 0x00100000
TEB, TEB_SIZE = 0x00040000, 0x00010000   # below every image: x86 images start at 0x00400000
TLS_BLOCK, TLS_SIZE = 0x00050000, 0x00040000
GDT = 0x00090000
STUBS, STUBS_SIZE = 0x000A0000, 0x00010000
# x86: a fake returning a float leaves it in ST0 through `fld dword ptr [FLOAT_RESULT]; ret`
FLOAT_RESULT_STUB, FLOAT_RESULT = STUBS + STUBS_SIZE - 0x20, STUBS + STUBS_SIZE - 0x10
SCRATCH, SCRATCH_SIZE = 0x30000000, 0x00100000  # clear of every image (x86 exes span 0x00400000 up)
# Program data the setup places outside the images (what a global pointer points to): writes here are compared, as
# writes to the images' globals are
OBJECTS, OBJECTS_SIZE = 0x31000000, 0x00200000
INSTRUCTION_LIMIT = 20_000_000  # a runaway guard; -O0 reconstructions take several times the original's instructions
MASK64 = (1 << 64) - 1
INT_REGS64 = (X.UC_X86_REG_RCX, X.UC_X86_REG_RDX, X.UC_X86_REG_R8, X.UC_X86_REG_R9)
XMM_REGS = (X.UC_X86_REG_XMM0, X.UC_X86_REG_XMM1, X.UC_X86_REG_XMM2, X.UC_X86_REG_XMM3)
RESET64 = [getattr(X, f"UC_X86_REG_{n}") for n in (
    "RAX", "RBX", "RCX", "RDX", "RSI", "RDI", "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15",
    *(f"XMM{i}" for i in range(16)))]
RESET32 = [getattr(X, f"UC_X86_REG_{n}") for n in (
    "EAX", "EBX", "ECX", "EDX", "ESI", "EDI", *(f"XMM{i}" for i in range(8)))]


def _page(n):
    return (n + 0xFFF) & ~0xFFF


class Image:
    def __init__(self, path: Path):
        self.pe = pefile.PE(str(path), fast_load=True)
        self.pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY[d] for d in (
            "IMAGE_DIRECTORY_ENTRY_EXPORT", "IMAGE_DIRECTORY_ENTRY_IMPORT", "IMAGE_DIRECTORY_ENTRY_TLS")])
        self.x64 = self.pe.FILE_HEADER.Machine == 0x8664
        self.base = self.pe.OPTIONAL_HEADER.ImageBase
        self.size = _page(self.pe.OPTIONAL_HEADER.SizeOfImage)
        self.data = self.pe.get_memory_mapped_image()
        self.exports = {}
        if hasattr(self.pe, "DIRECTORY_ENTRY_EXPORT"):
            for e in self.pe.DIRECTORY_ENTRY_EXPORT.symbols:
                self.exports[e.name.decode()] = self.base + e.address
        self.iat = {}  # import address table slot va -> "dll!name"
        for d in getattr(self.pe, "DIRECTORY_ENTRY_IMPORT", []):
            for imp in d.imports:
                name = imp.name.decode() if imp.name else f"#{imp.ordinal}"
                self.iat[imp.address] = f"{d.dll.decode().lower()}!{name}"
        self.rdata = [(self.base + s.VirtualAddress, self.base + s.VirtualAddress + s.Misc_VirtualSize)
                      for s in self.pe.sections if s.Name.rstrip(b"\0") == b".rdata"]
        self.tls = None  # (template bytes, va of the TLS index variable)
        if hasattr(self.pe, "DIRECTORY_ENTRY_TLS"):
            t = self.pe.DIRECTORY_ENTRY_TLS.struct
            start, end = t.StartAddressOfRawData - self.base, t.EndAddressOfRawData - self.base
            self.tls = (bytes(self.data[start:end]) + b"\0" * t.SizeOfZeroFill, t.AddressOfIndex)


def original_exe(build: str) -> Path:
    """$T6_ORIGINALS/<build>.exe (default originals/), verified against target.toml."""
    want = symbols.build_spec(build)["exe"]["sha256"]
    path = Path(os.environ.get("T6_ORIGINALS", ROOT / "originals")) / f"{build}.exe"
    if not path.exists():
        raise SystemExit(f"missing {path}: run tools/fetch_originals.py {build} {path.parent} --only exe")
    if hashlib.sha256(path.read_bytes()).hexdigest() != want:
        raise SystemExit(f"{path} does not match target.toml sha256")
    return path


def _descriptor(base, limit, access, flags):
    """An x86 segment descriptor (8 bytes)."""
    return struct.pack("<HHBBBB", limit & 0xFFFF, base & 0xFFFF, (base >> 16) & 0xFF, access,
                       ((limit >> 16) & 0x0F) | (flags << 4), (base >> 24) & 0xFF)


def _st0(mu):
    """ST0 as a Python float: the x87 extended value the caller sees, unrounded to float or double."""
    top = (mu.reg_read(X.UC_X86_REG_FPSW) >> 11) & 7
    mant, se = mu.reg_read(X.UC_X86_REG_FP0 + top)
    sign = -1.0 if se & 0x8000 else 1.0
    exp = se & 0x7FFF
    if exp == 0 and mant == 0:
        return math.copysign(0.0, sign)
    if exp == 0x7FFF:
        return sign * math.inf if (mant << 1) & MASK64 == 0 else math.nan
    return sign * mant / (1 << 63) * 2.0 ** (exp - 16383)


class Writer:
    """Sets up state for one call: writes into the images are undone afterwards, and scratch memory (strings,
    argument arrays) is handed out from a region cleared before every call."""

    def __init__(self, machine):
        self.m = machine
        self.next = SCRATCH

    def write(self, va, data: bytes):
        self.m._setup_write(va, bytes(data))

    def u8(self, va, v):
        self.write(va, bytes([v & 0xFF]))

    def u32(self, va, v):
        self.write(va, struct.pack("<I", v & 0xFFFFFFFF))

    def u64(self, va, v):
        self.write(va, struct.pack("<Q", v & MASK64))

    def f32(self, va, x):
        self.write(va, struct.pack("<f", x))

    def ptr(self, va, v):
        self.write(va, struct.pack("<Q" if self.m.x64 else "<I", v))

    def alloc(self, data: bytes) -> int:
        va = self.next
        self.m.mu.mem_write(va, bytes(data))
        self.next = (va + len(data) + 15) & ~15
        return va

    def cstr(self, s) -> int:
        return self.alloc((s.encode("latin1") if isinstance(s, str) else s) + b"\0")


def _crt_args(mu, n):
    return [mu.reg_read(r) for r in INT_REGS64[:n]]


def _crt_cstr(mu, va):
    out = bytearray()
    while True:
        chunk = bytes(mu.mem_read(va + len(out), 64))
        if b"\0" in chunk:
            return bytes(out + chunk[:chunk.index(b"\0")])
        out += chunk


# The models that write memory write as the program would (Machine._emulated_write): journaled and compared, like
# the writes of the original's own inlined copies. They take the machine as well as the emulator.
def _crt_memset(m, mu):
    dst, c, n = _crt_args(mu, 3)
    m._emulated_write(dst, bytes([c & 0xFF]) * n)
    mu.reg_write(X.UC_X86_REG_RAX, dst)


def _crt_memmove(m, mu):
    dst, src, n = _crt_args(mu, 3)
    m._emulated_write(dst, bytes(mu.mem_read(src, n)))
    mu.reg_write(X.UC_X86_REG_RAX, dst)


def _crt_strlen(mu):
    mu.reg_write(X.UC_X86_REG_RAX, len(_crt_cstr(mu, _crt_args(mu, 1)[0])))


def _crt_strchr(mu):
    s, c = _crt_args(mu, 2)
    text, c = _crt_cstr(mu, s) + b"\0", c & 0xFF
    i = text.find(bytes([c]))
    mu.reg_write(X.UC_X86_REG_RAX, s + i if i >= 0 else 0)


def _crt_atoi(mu):
    """UCRT atoi: strtol's parse (leading whitespace, a sign, decimal digits), saturating at the int range."""
    text = _crt_cstr(mu, _crt_args(mu, 1)[0]).lstrip(b" \t\n\v\f\r")
    sign = -1 if text[:1] == b"-" else 1
    if text[:1] in (b"-", b"+"):
        text = text[1:]
    digits = len(text) - len(text.lstrip(b"0123456789"))
    v = sign * int(text[:digits] or b"0")
    mu.reg_write(X.UC_X86_REG_RAX, max(-2**31, min(2**31 - 1, v)) & 0xFFFFFFFF)


CRT_WRITERS = (_crt_memset, _crt_memmove)
CRT_MODELS_X64 = {"vcruntime140.dll!memset": _crt_memset, "vcruntime140.dll!memcpy": _crt_memmove,
                  "vcruntime140.dll!memmove": _crt_memmove, "vcruntime140.dll!strchr": _crt_strchr,
                  "api-ms-win-crt-string-l1-1-0.dll!strlen": _crt_strlen,
                  "api-ms-win-crt-convert-l1-1-0.dll!atoi": _crt_atoi}


class Machine:
    def __init__(self, *images: Image, large_heap=False):
        self.images = images
        self.x64 = images[0].x64
        # Opt-in for large x64 video-plane buffers; preserve all old addresses.
        if large_heap and not self.x64:
            raise ValueError("large argument arena is x64-only")
        self.heap, self.heap_size = (0x32000000, 0x00800000) if large_heap else (HEAP, HEAP_SIZE)
        mu = self.mu = Uc(UC_ARCH_X86, UC_MODE_64 if self.x64 else UC_MODE_32)
        for img in images:
            mu.mem_map(img.base, img.size)
            mu.mem_write(img.base, img.data[:img.size])
        for va, size in ((STACK, STACK_SIZE), (self.heap, self.heap_size), (SENTINEL, 0x1000), (TEB, TEB_SIZE),
                         (TLS_BLOCK, TLS_SIZE), (STUBS, STUBS_SIZE), (SCRATCH, SCRATCH_SIZE),
                         (OBJECTS, OBJECTS_SIZE)):
            mu.mem_map(va, size)
        exe = images[0]
        self._tls_template = exe.tls[0] if exe.tls else b""
        if len(self._tls_template) > TLS_SIZE:
            raise SystemExit("TLS template larger than the harness region")
        if self.x64:
            mu.mem_write(TEB + 0x08, struct.pack("<Q", STACK + STACK_SIZE))  # NT_TIB.StackBase
            mu.mem_write(TEB + 0x10, struct.pack("<Q", STACK))               # NT_TIB.StackLimit
            mu.mem_write(TEB + 0x30, struct.pack("<Q", TEB))                 # NT_TIB.Self
            mu.mem_write(TEB + 0x60, struct.pack("<Q", TEB + 0x2000))        # PEB
            mu.mem_write(TEB + 0x2000 + 0x10, struct.pack("<Q", exe.base))   # PEB.ImageBaseAddress
            mu.mem_write(TEB + 0x58, struct.pack("<Q", TEB + 0x3000))        # TLS array: slot 0 -> block
            mu.mem_write(TEB + 0x3000, struct.pack("<Q", TLS_BLOCK))
            mu.msr_write(0xC0000101, TEB)                                     # IA32_GS_BASE
        else:
            index = 0
            if exe.tls:
                off = exe.tls[1] - exe.base
                index = struct.unpack("<I", bytes(exe.data[off:off + 4]))[0]
            mu.mem_write(TEB + 0x04, struct.pack("<I", STACK + STACK_SIZE))   # NT_TIB.StackBase
            mu.mem_write(TEB + 0x08, struct.pack("<I", STACK))                # NT_TIB.StackLimit
            mu.mem_write(TEB + 0x18, struct.pack("<I", TEB))                  # NT_TIB.Self
            mu.mem_write(TEB + 0x30, struct.pack("<I", TEB + 0x2000))         # PEB
            mu.mem_write(TEB + 0x2000 + 0x08, struct.pack("<I", exe.base))    # PEB.ImageBaseAddress
            mu.mem_write(TEB + 0x2C, struct.pack("<I", TEB + 0x3000))         # TLS array
            mu.mem_write(TEB + 0x3000 + 4 * index, struct.pack("<I", TLS_BLOCK))
            # 32-bit mode ignores FS_BASE: FS selects a GDT descriptor based on the TEB; the rest are flat
            mu.mem_map(GDT, 0x1000)
            mu.mem_write(GDT + 8 * 1, _descriptor(0, 0xFFFFF, access=0x9A, flags=0xC))
            mu.mem_write(GDT + 8 * 2, _descriptor(0, 0xFFFFF, access=0x92, flags=0xC))
            mu.mem_write(GDT + 8 * 3, _descriptor(TEB, TEB_SIZE - 1, access=0x92, flags=0x4))
            mu.reg_write(X.UC_X86_REG_GDTR, (0, GDT, 8 * 8 - 1, 0))
            mu.reg_write(X.UC_X86_REG_CS, 1 << 3)
            for reg in (X.UC_X86_REG_DS, X.UC_X86_REG_ES, X.UC_X86_REG_SS, X.UC_X86_REG_GS):
                mu.reg_write(reg, 2 << 3)
            mu.reg_write(X.UC_X86_REG_FS, 3 << 3)
        # import stubs: `xor eax, eax; ret`, with Python hooked on the ret
        self._stub_names, self._import_fns = {}, {}
        if self.x64:
            # the Store build links the CRT dynamically: its own code inlines most of these, the reconstruction
            # (and some original code) calls them
            for name, fn in CRT_MODELS_X64.items():
                self.on_import(name, (lambda f: lambda mu: f(self, mu))(fn) if fn in CRT_WRITERS else fn)
        for img in images:
            for slot, name in img.iat.items():
                stub = STUBS + 0x10 * len(self._stub_names)
                self._stub_names[stub + 2] = name
                mu.mem_write(stub, b"\x31\xc0\xc3")
                mu.mem_write(slot, struct.pack("<Q" if self.x64 else "<I", stub))
        mu.mem_write(FLOAT_RESULT_STUB, b"\xd9\x05" + struct.pack("<I", FLOAT_RESULT & 0xFFFFFFFF) + b"\xc3")
        mu.hook_add(UC_HOOK_CODE, self._on_stub, begin=STUBS, end=STUBS + STUBS_SIZE - 1)
        self.imports_called = []
        self._ranges = [(img.base, img.base + img.size) for img in images]
        self._ranges += [(TLS_BLOCK, TLS_BLOCK + TLS_SIZE), (OBJECTS, OBJECTS + OBJECTS_SIZE)]
        self._journal, self._setup_journal = {}, {}
        mu.hook_add(UC_HOOK_MEM_WRITE, self._on_write)
        self.tsc = 0
        self._hooked, self._fakes, self._observe, self._at, self.observed = set(), {}, {}, {}, []
        self._standing = {}  # fakes for every call (provide)
        self._rdtsc_cb = uccallback(mu, HOOK_INSN_CPUID_CFUNC)(self._on_rdtsc)
        for insn in (X.UC_X86_INS_RDTSC, X.UC_X86_INS_RDTSCP):
            mu._Uc__do_hook_add(UC_HOOK_INSN, self._rdtsc_cb, 1, 0, insn)

    # -- registers that differ by architecture
    @property
    def _sp(self):
        return X.UC_X86_REG_RSP if self.x64 else X.UC_X86_REG_ESP

    @property
    def _ip(self):
        return X.UC_X86_REG_RIP if self.x64 else X.UC_X86_REG_EIP

    @property
    def _ax(self):
        return X.UC_X86_REG_RAX if self.x64 else X.UC_X86_REG_EAX

    def _ptr(self, va):
        width = 8 if self.x64 else 4
        return struct.unpack("<Q" if self.x64 else "<I", bytes(self.mu.mem_read(va, width)))[0]

    def _on_rdtsc(self, mu, user):
        self.tsc += 0x400
        mu.reg_write(self._ax, self.tsc & 0xFFFFFFFF)
        mu.reg_write(X.UC_X86_REG_RDX if self.x64 else X.UC_X86_REG_EDX, self.tsc >> 32)
        return 1  # skip the real instruction

    def _on_write(self, mu, access, address, size, value, user):
        for lo, hi in self._ranges:
            if lo <= address < hi:
                for a in range(address, address + size):
                    if a not in self._journal:
                        self._journal[a] = bytes(mu.mem_read(a, 1))
                return

    def _emulated_write(self, va, data):
        """A write made for the program by Python (a fake or an import): journaled as _on_write does."""
        for lo, hi in self._ranges:
            a0, a1 = max(va, lo), min(va + len(data), hi)
            if a0 < a1:
                old = bytes(self.mu.mem_read(a0, a1 - a0))
                for i in range(a1 - a0):
                    self._journal.setdefault(a0 + i, old[i:i + 1])
        self.mu.mem_write(va, bytes(data))

    def end_call(self):
        """From a fake or an import model: stop the call here as if it had returned. For a function that never
        returns (a thread's loop), the test decides how far both runs go; the return value is then not compared."""
        self._ended = True
        self.mu.emu_stop()

    def program_write(self, va, data):
        """For a fake standing in for a function: write on the program's behalf, journaled and compared."""
        self._emulated_write(va, data)

    def _setup_write(self, va, data):
        for i in range(len(data)):
            self._setup_journal.setdefault(va + i, bytes(self.mu.mem_read(va + i, 1)))
        self.mu.mem_write(va, data)

    def read_cstr(self, va, limit=512):
        try:
            data = bytes(self.mu.mem_read(va, limit))
        except UcError:
            return f"<unreadable {va:#x}>"
        return data.split(b"\0", 1)[0]

    def import_stub(self, name):
        """Entry address of the stub behind import `name` ("kernel32.dll!Sleep", any case), or None."""
        if not hasattr(self, "_stub_by_name"):
            self._stub_by_name = {n.lower(): a - 2 for a, n in self._stub_names.items()}
        return self._stub_by_name.get(name.lower())

    def on_import(self, name, fn, nargs=0):
        """fn(mu) runs when import `name` is called; it may set the result register. On x86 nargs is the number of
        argument dwords the (stdcall) import pops; give 0 for a cdecl import."""
        self._import_fns[name.lower()] = (fn, nargs)

    def _on_stub(self, mu, address, size, user):
        name = self._stub_names.get(address)
        if name is None:
            return
        self.imports_called.append(name)
        hooked = self._import_fns.get(name.lower())
        if hooked is None:
            self._stub_fault = f"fault: import {name} called with no model (Machine.on_import)"
            mu.emu_stop()
            return
        fn, nargs = hooked
        fn(mu)
        if not self.x64 and nargs:  # stdcall: pop the arguments as the real import would
            esp = mu.reg_read(X.UC_X86_REG_ESP)
            ret = self._ptr(esp)
            mu.reg_write(X.UC_X86_REG_ESP, esp + 4 + 4 * nargs)
            mu.reg_write(X.UC_X86_REG_EIP, ret)

    def provide(self, va, fn):
        """Replace the function at va with fn(mu) -> return value in every call (as `fakes` does for one)."""
        self._standing[va] = fn
        self._hook_at(va)

    def crt_thread_data(self, getptd_vas, ptmbcinfo_va, ptlocinfo_va):
        """x86 VC100 CRT: give the emulated thread its per-thread data. The CRT finds it through TlsGetValue and
        FlsGetValue and builds it on first use (FlsAlloc, calloc), none of which the harness models, so
        _getptd and _getptd_noexit return one _tiddata block instead. Its locale pointers are the process
        defaults, as for a thread that never called _configthreadlocale. Used by number formatting
        (_vsnprintf's %d)."""
        ptd = TEB + 0x8000
        block = bytearray(0x214)
        block[0x14:0x18] = struct.pack("<I", 1)                               # _holdrand
        block[0x68:0x6C] = bytes(self.mu.mem_read(ptmbcinfo_va, 4))           # ptmbcinfo
        block[0x6C:0x70] = bytes(self.mu.mem_read(ptlocinfo_va, 4))           # ptlocinfo
        self.mu.mem_write(ptd, bytes(block))
        for va in getptd_vas:
            self.provide(va, lambda mu: ptd)

    def _hook_at(self, va):
        if va not in self._hooked:
            self._hooked.add(va)
            self.mu.hook_add(UC_HOOK_CODE, self._on_hooked, begin=va, end=va)

    def _arg(self, mu, i, kind):
        """Argument i at a function's entry, as `kind` (see _on_hooked)."""
        sp = mu.reg_read(self._sp)
        if self.x64:
            if i < 4:
                return mu.reg_read(XMM_REGS[i]) if kind == "f" else mu.reg_read(INT_REGS64[i])
            return struct.unpack("<Q", bytes(mu.mem_read(sp + 8 + 8 * i, 8)))[0]
        return struct.unpack("<I", bytes(mu.mem_read(sp + 4 + 4 * i, 4)))[0]

    def _on_hooked(self, mu, address, size, user):
        run = self._at.get(address)
        if run:
            run(mu)
        spec = self._observe.get(address)
        if spec is not None:
            # one letter per argument: b = bool/char (low byte), i = 32-bit integer, q = pointer-sized, f = float
            # (its 32-bit pattern), s = C string pointer, x = not compared (e.g. a pointer into the caller's stack)
            got = []
            for i, kind in enumerate(spec):
                raw = self._arg(mu, i, kind)
                if kind == "x":
                    got.append(None)
                elif kind == "s":
                    got.append(self.read_cstr(raw))
                else:
                    got.append(raw & {"q": MASK64 if self.x64 else 0xFFFFFFFF, "b": 0xFF}.get(kind, 0xFFFFFFFF))
            self.observed.append((address, *got))
        fn = self._fakes.get(address) or self._standing.get(address)
        if fn:
            value = fn(mu)
            if isinstance(value, float):
                # a float result: ST0 on x86 (the stub loads it and returns), XMM0 on x64
                if self.x64:
                    mu.reg_write(X.UC_X86_REG_XMM0, struct.unpack("<I", struct.pack("<f", value))[0])
                else:
                    mu.mem_write(FLOAT_RESULT, struct.pack("<f", value))
                    mu.reg_write(self._ip, FLOAT_RESULT_STUB)
                    return
                value = None
            sp = mu.reg_read(self._sp)
            ret = self._ptr(sp)
            if value is not None:
                mu.reg_write(self._ax, value & (MASK64 if self.x64 else 0xFFFFFFFF))
            mu.reg_write(self._sp, sp + (8 if self.x64 else 4))  # cdecl / Win64: the caller pops arguments
            mu.reg_write(self._ip, ret)

    def call(self, va, args, ret="int", setup=None, fakes=None, observe=None, at=None, limit=INSTRUCTION_LIMIT,
             convention=None):
        """Call the function at va. ret: int, uint, int64, uint64, bool, char, uchar, short, ushort, float, double,
        void, or cstr (a returned char pointer, compared as its string).
        convention (x86): one entry per argument, "stack" or the register the function takes it in ("eax", "ecx",
        "edx", "ebx", "esi", "edi", "xmm0".."xmm3"): link-time code generation gives static-looking functions custom
        conventions. Register arguments are set after the reset; the rest are pushed as usual.
        Returns (value, [buffer contents after the call], {address: byte} written in mapped images, [observed]).
        A crash returns ("fault: ...", [], writes, observed)."""
        mu = self.mu
        mu.mem_write(self.heap, b"\0" * self.heap_size)
        mu.mem_write(SCRATCH, b"\0" * SCRATCH_SIZE)
        mu.mem_write(OBJECTS, b"\0" * OBJECTS_SIZE)
        # the stack the frames use starts from the same bytes for every call, so a local the code reads before writing
        # (an original bug: AimOutput's melee fields in CL_MouseMove) reads the same in both runs, not what the
        # previous call left
        mu.mem_write(STACK + STACK_SIZE - STACK_FILL, b"\xCC" * STACK_FILL)
        mu.mem_write(TLS_BLOCK, self._tls_template + b"\0" * (TLS_SIZE - len(self._tls_template)))
        self._setup_journal, self.imports_called, self._stub_fault = {}, [], None
        self._ended = False
        self.tsc = 0x100000000
        self._fakes, self._observe, self._at, self.observed = dict(fakes or {}), dict(observe or {}), dict(at or {}), []
        for a in list(self._fakes) + list(self._observe) + list(self._at):
            self._hook_at(a)
        if setup:
            setup(Writer(self))
        heap, bufs, slots = self.heap, [], []  # slot: (kind, value) with kind int, q, f, d
        for a in args:
            if isinstance(a, (bytes, bytearray)):
                if heap + _page(len(a) + 16) > self.heap + self.heap_size:
                    raise ValueError("argument buffers exceed oracle heap")
                mu.mem_write(heap, bytes(a))
                bufs.append((heap, len(a)))
                slots.append(("int", heap))
                heap += _page(len(a) + 16)
            elif isinstance(a, tuple):
                slots.append(a)
            else:
                slots.append(("int", a))
        self._journal = {}
        in_regs = []
        if convention:
            assert not self.x64 and len(convention) == len(slots), "convention: one entry per argument (x86)"
            in_regs = [(where, slot) for where, slot in zip(convention, slots) if where != "stack"]
            slots = [slot for where, slot in zip(convention, slots) if where == "stack"]
        if self.x64:
            for r in RESET64:
                mu.reg_write(r, 0)
            regs = [(k, struct.unpack("<I", struct.pack("<f", v))[0] if k == "f" else
                     struct.unpack("<Q", struct.pack("<d", v))[0] if k == "d" else v & MASK64) for k, v in slots]
            nstack = max(0, len(regs) - 4)
            sp = ((STACK + STACK_SIZE - 0x100 - (8 + 0x20 + 8 * nstack)) & ~0xF) - 8
            mu.mem_write(sp, struct.pack("<Q", SENTINEL))
            for i, (_, v) in enumerate(regs[4:]):
                mu.mem_write(sp + 8 + 0x20 + 8 * i, struct.pack("<Q", v))
            for i, (k, v) in enumerate(regs[:4]):
                if k in ("f", "d"):
                    mu.reg_write(XMM_REGS[i], v)
                mu.reg_write(INT_REGS64[i], v)  # MSVC also passes varargs floats in the integer register
            mu.reg_write(X.UC_X86_REG_RSP, sp)
            mu.reg_write(X.UC_X86_REG_RBP, 0)
        else:
            for r in RESET32:
                mu.reg_write(r, 0)
            words = []
            for k, v in slots:
                if k == "f":
                    words.append(struct.unpack("<I", struct.pack("<f", v))[0])
                elif k == "d":
                    words.extend(struct.unpack("<II", struct.pack("<d", v)))
                elif k == "q":
                    words.extend((v & 0xFFFFFFFF, (v >> 32) & 0xFFFFFFFF))
                else:
                    words.append(v & 0xFFFFFFFF)
            sp = STACK + STACK_SIZE - 0x100 - 4 * len(words)
            sp &= ~0xF
            for i, w in enumerate(words):
                mu.mem_write(sp + 4 * i, struct.pack("<I", w))
            sp -= 4
            mu.mem_write(sp, struct.pack("<I", SENTINEL))
            mu.reg_write(X.UC_X86_REG_ESP, sp)
            mu.reg_write(X.UC_X86_REG_EBP, 0)
            # x87 as Windows starts a thread: 53-bit precision, round to nearest, all exceptions masked; empty stack
            mu.reg_write(X.UC_X86_REG_FPCW, 0x27F)
            mu.reg_write(X.UC_X86_REG_FPSW, 0)
            mu.reg_write(X.UC_X86_REG_FPTAG, 0xFFFF)
            for where, (k, v) in in_regs:
                if where.startswith("xmm"):
                    bits = struct.unpack("<I", struct.pack("<f", v))[0] if k == "f" else v & 0xFFFFFFFF
                    mu.reg_write(getattr(X, f"UC_X86_REG_{where.upper()}"), bits)
                else:
                    mu.reg_write(getattr(X, f"UC_X86_REG_{where.upper()}"), v & 0xFFFFFFFF)
        mu.reg_write(X.UC_X86_REG_MXCSR, 0x1F80)
        trace = None
        if os.environ.get("T6_TRACE"):  # debugging aid: the last instructions before a fault
            from collections import deque
            trace = deque(maxlen=int(os.environ["T6_TRACE"]))
            trace_hook = mu.hook_add(UC_HOOK_CODE, lambda mu_, a, n, u: trace.append(a))
        try:
            mu.emu_start(va, SENTINEL, count=limit)
            fault = self._stub_fault or (None if mu.reg_read(self._ip) == SENTINEL or self._ended else
                                         f"fault: no return within {limit:,} instructions "
                                         f"(at {mu.reg_read(self._ip):08X})")
        except UcError as e:
            fault = f"fault: {e} at {mu.reg_read(self._ip):08X}"
        if trace is not None:
            mu.hook_del(trace_hook)
            if fault:
                print("trace:", " ".join(f"{a:X}" for a in trace), file=sys.stderr)
        # net changes only, as whole aligned qwords (run.py recognises stored pointers in them)
        changed = {a & ~7 for a in self._journal if bytes(mu.mem_read(a, 1)) != self._journal[a]}
        writes = {a + i: bytes(mu.mem_read(a + i, 1)) for a in sorted(changed) for i in range(8)}
        returned_str = None
        if ret == "cstr" and not fault:
            p = mu.reg_read(self._ax) & (MASK64 if self.x64 else 0xFFFFFFFF)
            returned_str = self.read_cstr(p) if p else None  # before rollback: it may point at a written global
        for a, old in list(self._journal.items()) + list(self._setup_journal.items()):
            mu.mem_write(a, old)
        self._journal, self._setup_journal = {}, {}
        observed, self._fakes, self._observe, self._at = self.observed, {}, {}, {}
        if fault:
            return fault, [], writes, observed
        ax = mu.reg_read(self._ax)
        if self.x64:
            r64 = ax
            xmm0 = mu.reg_read(X.UC_X86_REG_XMM0)
            fval = struct.unpack("<f", struct.pack("<I", xmm0 & 0xFFFFFFFF))[0]
            dval = struct.unpack("<d", struct.pack("<Q", xmm0 & MASK64))[0]
        else:
            r64 = (mu.reg_read(X.UC_X86_REG_EDX) << 32) | ax
            fval = dval = _st0(mu) if ret in ("float", "double") else None
        eax = r64 & 0xFFFFFFFF
        value = {
            "int": eax - (1 << 32) if eax & 0x80000000 else eax,
            "uint": eax,
            "int64": r64 - (1 << 64) if r64 >> 63 else r64,
            "uint64": r64,
            "bool": eax & 0xFF,
            "char": struct.unpack("<b", bytes([eax & 0xFF]))[0],
            "uchar": eax & 0xFF,
            "short": struct.unpack("<h", struct.pack("<H", eax & 0xFFFF))[0],
            "ushort": eax & 0xFFFF,
            "float": fval,
            "double": dval,
            "void": None,
            "cstr": returned_str,
        }[ret]
        return value, [bytes(mu.mem_read(a, n)) for a, n in bufs], writes, observed
