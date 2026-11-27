"""Native replay: the original Store code and the reconstruction run side by side in one Linux process.

The pinned original exe is mapped as an image (the Store build has no relocations and reaches its data RIP-relative,
so it runs at any address) and called through ms_abi function pointers; the reconstruction is the same source the
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


def map_image(path, sha256=STORE_MP_SHA256):
    """The exe's sections at their RVAs in one read/write/execute mapping."""
    data = pathlib.Path(path).read_bytes()
    if hashlib.sha256(data).hexdigest() != sha256:
        raise ValueError(f"{path}: not the pinned original")
    nt, = struct.unpack_from("<I", data, 60)
    count, = struct.unpack_from("<H", data, nt + 6)
    opt, = struct.unpack_from("<H", data, nt + 20)
    size, = struct.unpack_from("<I", data, nt + 0x50)
    mem = mmap.mmap(-1, size, prot=mmap.PROT_READ | mmap.PROT_WRITE | mmap.PROT_EXEC)
    for i in range(count):
        _, va, raw_size, raw = struct.unpack_from("<4I", data, nt + 24 + opt + 40 * i + 8)
        mem[va:va + raw_size] = data[raw:raw + raw_size]
    return mem


def base_of(mem):
    return ctypes.addressof(ctypes.c_char.from_buffer(mem))


def hook(mem, rva, target):
    """Replace the original function at rva with a jump to target (mov rax, target; jmp rax)."""
    mem[rva:rva + 12] = b"\x48\xb8" + struct.pack("<Q", target) + b"\xff\xe0"


def set_pointer(mem, rva, value):
    """Write a pointer into the image (an import address table slot, a global)."""
    struct.pack_into("<Q", mem, rva, value)


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
