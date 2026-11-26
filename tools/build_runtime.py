#!/usr/bin/env python3
"""Build the enhanced Store build as it is installed: two DLLs for the game folder.

    python3 tools/build_runtime.py --build t6mp_msstore
    python3 tools/build_runtime.py --build t6zm_msstore

- t6_enhanced.dll (MP) or t6_enhanced_zm.dll (ZM), with the runtime
  link's exe-first thunks, redirect table and checks (tools/runtime_link.py).
- XINPUT1_4.dll: the loader (runtime/loader): forwards XInput to the system DLL, selects the enhanced DLL
  for the supported exe, and applies its redirects.

Then the package is checked the way the loader will use it: the loader exports XInput's ordinals 2, 3 and 4,
and the enhanced DLL's redirect table holds, for each replaced function, an RVA inside the exe,
the exe's own first bytes there, and the address of an export of the same name. Installing is in
docs/enhanced-build.md.
"""
import argparse
import shutil
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tests" / "differential"))
TARGETS = {"t6mp_msstore": "t6_enhanced.dll", "t6zm_msstore": "t6_enhanced_zm.dll"}
LOADER = ROOT / "runtime" / "loader"
TRIPLE = "--target=x86_64-pc-windows-msvc"


def run(*args):
    subprocess.run([str(a) for a in args], check=True)


def build_loader(work, out):
    run("clang", TRIPLE, "-O2", "-ffreestanding", "-fno-builtin", "-fno-stack-protector", "-Wall", "-Werror",
        "-c", LOADER / "xinput_proxy.c", "-o", work / "xinput_proxy.obj")
    run("clang", TRIPLE, "-c", LOADER / "xinput_stubs.s", "-o", work / "xinput_stubs.obj")
    run("llvm-dlltool", "-m", "i386:x86-64", "-d", LOADER / "kernel32.def", "-l", work / "kernel32.lib")
    run("lld-link", "/dll", "/Brepro", "/nodefaultlib", "/entry:ProxyMain", f"/def:{LOADER / 'xinput.def'}",
        f"/out:{out / 'XINPUT1_4.dll'}", work / "xinput_proxy.obj", work / "xinput_stubs.obj", work / "kernel32.lib")


def check(exe, out, dll_name):
    import pefile
    from symbols import scoped_name
    loader = pefile.PE(str(out / "XINPUT1_4.dll"))
    ordinals = {e.ordinal: (e.name or b"").decode() for e in loader.DIRECTORY_ENTRY_EXPORT.symbols}
    for o, name in ((2, "XInputGetState"), (3, "XInputSetState"), (4, "XInputGetCapabilities")):
        assert ordinals.get(o) == name, f"loader: ordinal {o} is {ordinals.get(o)!r}, not {name}"

    base, size = exe.OPTIONAL_HEADER.ImageBase, exe.OPTIONAL_HEADER.SizeOfImage
    dll = pefile.PE(str(out / dll_name))
    dbase = dll.OPTIONAL_HEADER.ImageBase
    exports = {(e.name or b"").decode(): dbase + e.address for e in dll.DIRECTORY_ENTRY_EXPORT.symbols}
    for name in ("InitializeApiImpl", "InitializeApiImplEx",
                 "UninitializeApiImpl", "QueryApiImpl", "XErrorReport"):
        assert name in exports, f"{dll_name}: missing offline export {name}"
    by_address = {}
    for name, va in exports.items():
        by_address.setdefault(va, []).append(name)
    table = exports["t6_redirects"]
    count = struct.unpack("<Q", dll.get_data(table - dbase, 8))[0]
    assert count, f"{dll_name}: empty redirect table"
    identity = exports["t6_redirects_identity"]
    assert identity == table + 8 + 32 * count, f"{dll_name}: invalid redirect table length"
    assert struct.unpack("<II", dll.get_data(identity - dbase, 8)) == (exe.FILE_HEADER.TimeDateStamp, size), (
        f"{dll_name}: redirect table targets another executable"
    )
    lines = []
    redirects = set()
    for i in range(count):
        rva, target = struct.unpack("<QQ", dll.get_data(table - dbase + 8 + 32 * i, 16))
        head = dll.get_data(table - dbase + 8 + 32 * i + 16, 16)
        assert rva + len(head) <= size, f"redirect {i}: RVA {rva:#x} outside the exe"
        assert exe.get_data(rva, 16) == head, f"redirect {i}: prologue differs from the exe at {base + rva:X}"
        names = by_address.get(target)
        assert names, f"redirect {i}: target {target:#x} is not an export"
        lines.append(f"{base + rva:X} -> {names[0]}")
        redirects.update(scoped_name(name) for name in names)
    for name in ("GdkSavePath", "GdkLoadRuntime", "Sys_ReportUnhandledException", "BdGetHostByName",
                 "BdSendTo", "Sys_ProbeTimeServer", "BdConnectStream", "WinInetOpenConnection",
                 "BdBeginWinHttpRequest", "XCurlCreateProvider", "XCurlAddRequest", "WinHttpInitializeRequest",
                 "BdHandleGdkAuthTask", "GdkUserCheckPrivilege", "BdInitNetStartParams"):
        assert name in redirects, f"service redirect {name} missing"
    return lines


def main():
    import pefile
    from harness import original_exe
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", choices=TARGETS, default="t6mp_msstore")
    parser.add_argument("--renderer-native", action="store_true", help="opt-in complete Store MP renderer registration replacement")
    args = parser.parse_args()
    build = args.build
    if args.renderer_native and build != "t6mp_msstore":
        parser.error("--renderer-native is only available for t6mp_msstore")
    exe = pefile.PE(str(original_exe(build)), fast_load=True)
    out = ROOT / "build" / build / "runtime"
    dll_name = TARGETS[build]
    run(sys.executable, ROOT / "tools" / "build_recon.py", "--build", build, "--enhanced",
        *(["--renderer-native"] if args.renderer_native else []))
    out.mkdir(parents=True, exist_ok=True)
    work = out / "obj"
    work.mkdir(exist_ok=True)
    shutil.copyfile(ROOT / "build" / build / "enhanced" / "recon.dll", out / dll_name)
    build_loader(work, out)
    for line in check(exe, out, dll_name):
        print("redirect", line)
    print(f"{out}: XINPUT1_4.dll, {dll_name}")


if __name__ == "__main__":
    main()
