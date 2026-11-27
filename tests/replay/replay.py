"""Native replay: the original Store code and the reconstruction run side by side in one Linux process.

The pinned original exe is mapped as an image at its own base address (the Store build has no relocations) and called through ms_abi function pointers; the reconstruction is the same source the
hybrid build compiles (src/), built natively for the test into a shared library together with the test's probe
(tests/replay/<unit>_probe.cpp). The probe supplies what the function under test calls, once for each side: hooks
patched over the original's callees, and plain definitions of the same functions for the reconstruction. Both sides
record what they did into one buffer and the test compares them byte for byte.

A test module (verify_<unit>.py) declares FUNCTIONS, [(build, va, name), ...] for what it verifies, and
main(exe_path). run.py runs them and writes build/<build>/replay-results.tsv, which tests/differential/run.py
--check-records accepts as passing tests alongside its own.
"""
import ctypes
import hashlib
import mmap
import pathlib
import shutil
import struct
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
STORE_MP_SHA256 = "cf80b8058bce7e00e4c61ff3d673308c1fcd8061f18fe66710803538eab4bc01"


class Image:
    """The mapped exe: indexable and sliceable like bytes, a buffer for struct and ctypes, a context manager."""

    def __init__(self, addr, size):
        self.addr, self.size = addr, size
        self.view = memoryview((ctypes.c_char * size).from_address(addr)).cast("B")

    def __getitem__(self, key):
        return self.view[key]

    def __setitem__(self, key, value):
        self.view[key] = value

    def __buffer__(self, flags):
        return self.view

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.view.release()
        _libc.munmap(ctypes.c_void_p(self.addr), ctypes.c_size_t(self.size))


_libc = ctypes.CDLL(None, use_errno=True)
_libc.mmap.restype = ctypes.c_void_p
_libc.mmap.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_long]
_libc.munmap.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
PROT_RWX, MAP_PRIVATE_ANON, MAP_FIXED_NOREPLACE = 7, 0x22, 0x100000


def map_image(path, sha256=STORE_MP_SHA256):
    """The exe's sections at their RVAs in one read/write/execute mapping at the exe's own base address: the Store
    build has no relocations, so the absolute pointers in its data (tables of strings and functions) are right only
    there. The code itself reaches its data RIP-relative."""
    data = pathlib.Path(path).read_bytes()
    if hashlib.sha256(data).hexdigest() != sha256:
        raise ValueError(f"{path}: not the pinned original")
    nt, = struct.unpack_from("<I", data, 60)
    count, = struct.unpack_from("<H", data, nt + 6)
    opt, = struct.unpack_from("<H", data, nt + 20)
    image_base, = struct.unpack_from("<Q", data, nt + 24 + 24)
    size, = struct.unpack_from("<I", data, nt + 0x50)
    addr = _libc.mmap(image_base, size, PROT_RWX, MAP_PRIVATE_ANON | MAP_FIXED_NOREPLACE, -1, 0)
    if addr != image_base:
        raise OSError(f"can't map the exe at its base {image_base:#x}: {ctypes.get_errno()}")
    mem = Image(addr, size)
    for i in range(count):
        _, va, raw_size, raw = struct.unpack_from("<4I", data, nt + 24 + opt + 40 * i + 8)
        mem[va:va + raw_size] = data[raw:raw + raw_size]
    return mem


def base_of(mem):
    return mem.addr


def hook(mem, rva, target):
    """Replace the original function at rva with a jump to target (mov rax, target; jmp rax)."""
    mem[rva:rva + 12] = b"\x48\xb8" + struct.pack("<Q", target) + b"\xff\xe0"


def set_pointer(mem, rva, value):
    """Write a pointer into the image (an import address table slot, a global)."""
    struct.pack_into("<Q", mem.view, rva, value)


def build(probe, out_dir, sources=(), defines=("T6_STORE",)):
    """Compile tests/replay/<probe> and the src/ files it tests (paths relative to src/, each its own translation
    unit, as in the hybrid build) into one shared library. A src/ file also holds functions and globals the test
    doesn't reach; whatever nothing in the library defines is linked as a weak symbol at one zeroed block (the C
    library's own definitions win), as the hybrid build sends it to the original."""
    out_dir = pathlib.Path(out_dir)
    so = out_dir / (pathlib.Path(probe).stem + ".so")
    cxx = shutil.which("clang++") or "g++"
    flags = ["-std=c++17", "-O2", "-ffp-contract=off", "-fms-extensions", "-Wall", "-Wextra", "-Werror",
             "-Wno-unused-parameter", "-Wno-unused-function", "-fPIC", "-DT6_NO_ASSERTS", "-DT6_REPLAY",
             *[f"-D{d}" for d in defines], f"-I{ROOT / 'include'}", f"-I{ROOT / 'src'}",
             # after the system headers: t6_assert.h, t6_static.h; the C library itself is the host's
             f"-idirafter{ROOT / 'tools' / 'recon' / 'include'}"]
    if "clang" in cxx:
        flags.insert(0, "-fdeclspec")
    objects = []
    for i, src in enumerate([ROOT / "tests" / "replay" / probe, *[ROOT / "src" / f for f in sources]]):
        obj = out_dir / f"{i}_{src.stem}.o"
        subprocess.run([cxx, *flags, "-c", str(src), "-o", str(obj)], check=True)
        objects.append(obj)
    nm = shutil.which("llvm-nm") or "nm"
    defined, undefined = set(), set()
    for obj in objects:
        for line in subprocess.check_output([nm, str(obj)], text=True).splitlines():
            parts = line.split()
            if len(parts) == 2 and parts[0] in ("U", "w"):
                undefined.add(parts[1])
            elif len(parts) == 3 and parts[1] not in ("U", "w"):
                defined.add(parts[2])
    weak = out_dir / "weak.s"
    # weak definitions at one zeroed block: a global reads as zero, and nothing the test runs may call a function
    weak.write_text('.section .note.GNU-stack,"",@progbits\n.bss\n.p2align 12\nt6_replay_unlinked: .zero 4096\n' +
                    "".join(f'.weak "{s}"\n.set "{s}", t6_replay_unlinked\n' for s in sorted(undefined - defined)))
    subprocess.run([cxx, "-shared", "-fPIC", *map(str, objects), str(weak), "-o", str(so)], check=True)
    return ctypes.CDLL(str(so))


def tempdir():
    return tempfile.TemporaryDirectory()
