#!/usr/bin/env python3
"""Differential XCurl internal getinfo field selector against pinned original.

Original FUN_18000AD58 includes Windows critical-section and telemetry wrappers.
This test deliberately models only those external lock calls as no-ops so the
original's internal option-dispatch logic is exercised, then compares return
code and all output-buffer bytes against reconstructed native source.
This is NOT a full curl_easy_getinfo wrapper or HTTP implementation.
"""
import ctypes
import hashlib
import random
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/differential"))
from harness import Image, Machine, OBJECTS  # noqa: E402

SHA256 = "65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551"
RVA = 0xAD58
OUTPUT = OBJECTS + 0x200
FILL = bytes([0xA5]) * 8
OPTIONS = (
    0x100001, 0x100012, 0x100015, 0x100031,
    0x200002, 0x20000B, 0x20000D, 0x200019, 0x20002E, 0x200030,
    0x300008, 0x30000F, 0x300010,
    0x600008, 0x60000F, 0x600010, 0x600039,
    0, 0xDEAD, 0x700001, 0x20002F,
)


def main():
    original = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "originals/dlls/XCurl.dll"
    libpath = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / "build/xcurl_getinfo_test.so"
    if hashlib.sha256(original.read_bytes()).hexdigest() != SHA256:
        raise SystemExit("Pinned XCurl DLL SHA256 mismatch")
    img = Image(original)
    assert img.base == 0x180000000
    m = Machine(img)
    locks = set()
    for name in img.iat.values():
        if name.lower().endswith(("!entercriticalsection", "!leavecriticalsection")):
            m.on_import(name, lambda mu: None)
            locks.add(name)
    if not any("EnterCriticalSection" in s for s in locks):
        raise AssertionError("Expected original Windows critical-section imports missing")

    lib = ctypes.CDLL(str(libpath.resolve()))
    fn = lib.xcurl_easy_getinfo_unlocked
    fn.argtypes = (ctypes.c_void_p, ctypes.c_int, ctypes.POINTER(ctypes.c_void_p))
    fn.restype = ctypes.c_uint32

    rng = random.Random(0xAD58)
    cases = [bytes(0x240), bytes([0xFF])*0x240, bytes(range(256))*2]
    cases += [rng.randbytes(0x240) for _ in range(16)]
    checks = 0
    for case_index, handle in enumerate(cases):
        for opt in OPTIONS:
            def setup(writer):
                writer.write(OUTPUT, FILL)
            ptr_slot = struct.pack("<Q", OUTPUT)
            ret, buffers, writes, _ = m.call(
                img.base + RVA, [handle, opt, ptr_slot], "uint", setup=setup)
            if isinstance(ret, str) and ret.startswith("fault:"):
                raise AssertionError(f"Original fault {ret!r} case={case_index} opt={opt:#x}")
            original_output = b"".join(
                writes.get(OUTPUT+i, FILL[i:i+1]) for i in range(8))

            native_handle = ctypes.create_string_buffer(handle, len(handle))
            native_output = ctypes.create_string_buffer(FILL, len(FILL))
            output_ptr = ctypes.c_void_p(ctypes.addressof(native_output))
            actual_ret = fn(native_handle, opt, ctypes.byref(output_ptr))
            actual_output = bytes(native_output)
            if ret != actual_ret or original_output != actual_output:
                raise AssertionError(
                    f"XCurl getinfo mismatch case={case_index} opt={opt:#x}: "
                    f"original return={ret} output={original_output.hex()} "
                    f"recon return={actual_ret} output={actual_output.hex()}")
            if buffers[0] != handle:
                raise AssertionError("Input handle mutated")
            checks += 1
    print(f"XCurl 0x{RVA:X} getinfo core differential PASS: {checks} cases")


if __name__ == "__main__":
    main()
