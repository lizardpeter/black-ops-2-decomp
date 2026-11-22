#!/usr/bin/env python3
"""Build the enhanced Store build as it is installed: two DLLs for the game folder.

    python3 tools/build_runtime.py            # -> build/t6mp_msstore/runtime/{XINPUT1_4.dll, t6_enhanced.dll}

- t6_enhanced.dll: the enhanced Store DLL (tools/build_recon.py --build t6mp_msstore --enhanced), with the runtime
  link's exe-first thunks, redirect table and checks (tools/runtime_link.py).
- XINPUT1_4.dll: the loader (runtime/loader): forwards XInput to the system DLL and, in the expected t6mp.exe,
  loads t6_enhanced.dll and applies its redirects.

Then the package is checked the way the loader will use it: the loader exports XInput's ordinals 2, 3 and 4 (what
t6mp.exe imports), and t6_enhanced.dll's redirect table holds, for each replaced function, an RVA inside the exe,
the exe's own first bytes there, and the address of an export of the same name. Installing is in
docs/enhanced-build.md.
"""
import shutil
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tests" / "differential"))
BUILD = "t6mp_msstore"
OUT = ROOT / "build" / BUILD / "runtime"
LOADER = ROOT / "runtime" / "loader"
TRIPLE = "--target=x86_64-pc-windows-msvc"


def run(*args):
    subprocess.run([str(a) for a in args], check=True)


def build_loader(work):
    run("clang", TRIPLE, "-O2", "-ffreestanding", "-fno-builtin", "-fno-stack-protector", "-Wall", "-Werror",
        "-c", LOADER / "xinput_proxy.c", "-o", work / "xinput_proxy.obj")
    run("clang", TRIPLE, "-c", LOADER / "xinput_stubs.s", "-o", work / "xinput_stubs.obj")
    run("llvm-dlltool", "-m", "i386:x86-64", "-d", LOADER / "kernel32.def", "-l", work / "kernel32.lib")
    run("lld-link", "/dll", "/nodefaultlib", "/entry:ProxyMain", f"/def:{LOADER / 'xinput.def'}",
        f"/out:{OUT / 'XINPUT1_4.dll'}", work / "xinput_proxy.obj", work / "xinput_stubs.obj", work / "kernel32.lib")


def check(exe_path):
    import pefile
    loader = pefile.PE(str(OUT / "XINPUT1_4.dll"))
    ordinals = {e.ordinal: (e.name or b"").decode() for e in loader.DIRECTORY_ENTRY_EXPORT.symbols}
    for o, name in ((2, "XInputGetState"), (3, "XInputSetState"), (4, "XInputGetCapabilities")):
        assert ordinals.get(o) == name, f"loader: ordinal {o} is {ordinals.get(o)!r}, not {name}"

    exe = pefile.PE(str(exe_path), fast_load=True)
    base, size = exe.OPTIONAL_HEADER.ImageBase, exe.OPTIONAL_HEADER.SizeOfImage
    loader_src = (LOADER / "xinput_proxy.c").read_text()
    assert f"0x{exe.FILE_HEADER.TimeDateStamp:08X}u" in loader_src and f"0x{size:08X}u" in loader_src, \
        "loader: T6MP_TIMESTAMP / T6MP_IMAGE_SIZE don't match the pinned t6mp.exe"

    dll = pefile.PE(str(OUT / "t6_enhanced.dll"))
    dbase = dll.OPTIONAL_HEADER.ImageBase
    exports = {(e.name or b"").decode(): dbase + e.address for e in dll.DIRECTORY_ENTRY_EXPORT.symbols}
    by_address = {}
    for name, va in exports.items():
        by_address.setdefault(va, []).append(name)
    table = exports["t6_redirects"]
    count = struct.unpack("<Q", dll.get_data(table - dbase, 8))[0]
    assert count, "t6_enhanced.dll: empty redirect table"
    out = []
    for i in range(count):
        rva, target = struct.unpack("<QQ", dll.get_data(table - dbase + 8 + 32 * i, 16))
        head = dll.get_data(table - dbase + 8 + 32 * i + 16, 16)
        assert rva < size, f"redirect {i}: RVA {rva:#x} outside the exe"
        assert exe.get_data(rva, 16) == head, f"redirect {i}: prologue differs from the exe at {base + rva:X}"
        names = by_address.get(target)
        assert names, f"redirect {i}: target {target:#x} is not an export"
        out.append(f"{base + rva:X} -> {names[0]}")
    return out


def main():
    from harness import original_exe
    run(sys.executable, ROOT / "tools" / "build_recon.py", "--build", BUILD, "--enhanced")
    OUT.mkdir(parents=True, exist_ok=True)
    work = OUT / "obj"
    work.mkdir(exist_ok=True)
    shutil.copyfile(ROOT / "build" / BUILD / "enhanced" / "recon.dll", OUT / "t6_enhanced.dll")
    build_loader(work)
    for line in check(original_exe(BUILD)):
        print("redirect", line)
    print(f"{OUT}: XINPUT1_4.dll, t6_enhanced.dll")


if __name__ == "__main__":
    main()
