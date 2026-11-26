#!/usr/bin/env python3
"""Compare the recovered renderer entry prefix against original Store machine code.

Only the recovered entry section is executed. Cut execution before the next
original instruction; restore exactly the nonvolatile state saved by its prologue.
The Windows stack-probe helper is replaced by RET, preserving RAX's allocation
size; Linux supplies the stack for this isolated test.
"""
import ctypes as C
import pathlib
import struct
import subprocess
import sys
import tempfile

from verify_store_cgame_registration import map_image, ROOT

TARGETS = [0x763730, 0x46b8b0, 0x46c1a0, 0x46bf10, 0x46c4c0, 0x46c650, 0x46caf0, 0x46cbd0, 0x46c840]
GLOBALS = [0x27e71d0, 0x27e6f88, 0x27e6fa0, 0x27e6fd0, 0x27e6f70, 0x27e7190, 0x27e7090, 0x27e7188, 0x27e6ef8, 0x27e6f10, 0x27e6fe0, 0x27e6f68, 0x27e6ea0, 0x27e7288, 0x27e7278, 0x27e7230, 0x27e7280, 0x27e71c8, 0x27e7118, 0x27e7170, 0x27e7080, 0x27e6f00, 0x27e7098, 0x27e70d0, 0x27e6fc0, 0x27e70f8, 0x27e6fb8, 0x27e7030, 0x27e71a8, 0x27e7258, 0x27e6e40, 0x27e6d10, 0x27e6d98, 0x27e6ca8, 0x27e6f28, 0x27e6bb0, 0x27e7108, 0x27e6e68, 0x27e7058, 0x27e6ef0, 0x27e7048, 0x27e7078, 0x27e6fa8, 0x27e7000, 0x27e70b0, 0x27e70e8, 0x27e7228, 0x27e7070, 0x27e71b0, 0x27e7148, 0x27e7178, 0x27e7028, 0x27e7168, 0x27e70a0, 0x27e6f18, 0x27e6cc8, 0x27e6d88, 0x27e7018]
CUT = 0x70f91f


def epilogue():
    # MOVAPS load XMM6..15 from the original aligned stack save slots.
    data = bytearray()
    for reg in range(6, 16):
        if reg >= 8:
            data.append(0x44)
        data.extend((0x0f, 0x28, 0x84 | ((reg & 7) << 3), 0x24))
        data.extend(struct.pack("<I", 0x2ba0 - (reg - 6) * 16))
    data.extend(bytes.fromhex("48 81 c4 b0 2b 00 00 41 5f 41 5e 41 5d 41 5c 5d"))
    data.extend(bytes.fromhex("48 8b 5c 24 08 48 8b 74 24 10 48 8b 7c 24 18 c3"))
    return data


def main():
    with tempfile.TemporaryDirectory() as tmp:
        so = pathlib.Path(tmp) / "renderer.so"
        subprocess.run(["g++", "-std=c++17", "-O2", "-Wall", "-Wextra", "-Werror",
                        "-shared", "-fPIC", str(ROOT / "tests/store_renderer_prefix_probe.cpp"),
                        "-o", str(so)], check=True)
        lib = C.CDLL(str(so))
        lib.RendererPrefixMock.argtypes = [C.c_uint]
        lib.RendererPrefixMock.restype = C.c_void_p
        lib.RendererPrefixSize.restype = C.c_uint
        lib.RendererPrefixRun.argtypes = [C.c_void_p, C.c_uint, C.c_void_p]
        lib.RendererPrefixRun.restype = None
        lib.RendererPrefixGlobals.argtypes = [C.c_void_p]
        lib.RendererPrefixGlobals.restype = None
        old = C.create_string_buffer(lib.RendererPrefixSize())
        new = C.create_string_buffer(lib.RendererPrefixSize())
        globals_ = (C.c_uint64 * len(GLOBALS))()
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base = C.addressof(C.c_char.from_buffer(mem))
            assert mem[CUT:CUT+6] == bytes.fromhex("8b 05 17 03 96 03")
            assert mem[0x70e250:0x70e25d] == bytes.fromhex("b8 b0 2b 00 00 e8 66 24 16 00 48 2b e0")
            for i, rva in enumerate(TARGETS):
                mem[rva:rva+12] = b"\x48\xb8" + struct.pack("<Q", lib.RendererPrefixMock(i if i<7 else i+1)) + b"\xff\xe0"
            mem[0x8706c0:0x8706c1] = b"\xc3"
            tail = epilogue()
            mem[CUT:CUT+len(tail)] = tail
            # Cover each lookup independently plus deterministic mixed patterns,
            # with both mode-byte values. Not exhaustive over 18 bits.
            masks = sorted({0, (1<<18)-1, *[1<<i for i in range(18)],
                            *[((i*0x9e3779b9) & ((1<<18)-1)) for i in range(1024)]})
            for mask in [m | (mode<<18) for m in masks for mode in range(2)]:
                mem[0x27e7331]=mask>>18
                for rva in GLOBALS:
                    struct.pack_into("<Q", mem, rva, 0)
                lib.RendererPrefixRun(base, mask, old)
                lib.RendererPrefixRun(None, mask, new)
                assert struct.unpack_from("<Q", old.raw)[0] == 102
                if old.raw != new.raw:
                    pos=next(i for i,(x,y) in enumerate(zip(old.raw,new.raw)) if x!=y)
                    if pos<8: raise AssertionError((mask,"event count",struct.unpack_from("<Q",old.raw),struct.unpack_from("<Q",new.raw)))
                    idx=(pos-8)//80
                    raise AssertionError((mask,idx,struct.unpack_from("<10Q",old.raw,8+80*idx),struct.unpack_from("<10Q",new.raw,8+80*idx)))
                lib.RendererPrefixGlobals(globals_)
                assert list(globals_) == [struct.unpack_from("<Q", mem, rva)[0] for rva in GLOBALS], mask
        print(f"{len(masks)*2} renderer prefix cases; 102 ordered events and 58 result slots per case")


if __name__ == "__main__":
    main()
