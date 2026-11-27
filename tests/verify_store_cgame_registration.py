#!/usr/bin/env python3
"""Differential replay of the Store MP project registration; no live game launched."""
import ctypes as C
import hashlib
import mmap
import pathlib
import struct
import subprocess
import sys
import tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
DIGEST = "cf80b8058bce7e00e4c61ff3d673308c1fcd8061f18fe66710803538eab4bc01"
TARGETS = [0x297eb0, 0x46b8b0, 0x46c1a0, 0x46bf10, 0x46c4c0, 0x46c650, 0x46c760, 0x46c840, 0x46cbd0, 0x46caf0, 0x46ca30, 0xfb7d0, 0x22ad80, 0xc8290, 0xba5a0, 0xf7b80, 0xf2230, 0x15df60, 0x179890, 0x61baf0, 0x101410]
GLOBALS = [0xd76888, 0xe53b20, 0xe4f838, 0xe53b60, 0xe53b68, 0xd78660, 0xe4f8e8, 0xd78658, 0xd78648, 0xd78638, 0xd78650, 0xd78628, 0xd78618, 0xe53a40, 0xe53a38, 0xe4baa8, 0xe4f828, 0xe4f890, 0xe53ae8, 0xe4f888, 0xe53b30, 0xe53ac8, 0xe4f8c0, 0xe4f898, 0xe4f8a0, 0xe4f8b8, 0xe4f8c8, 0xe4f8b0, 0xe4f8a8, 0xe4f8d0, 0xe53ae0, 0xe53a28, 0xe53b08, 0xe53b00, 0xe53b28, 0xe53b18, 0xe4bae0, 0xe53b80, 0xe49a38, 0xe4f870, 0xe4f860, 0xe4bac8, 0xe4ba98, 0xe4ba78, 0xe4ba90, 0xe4bae8, 0xe4bad0, 0xe4f878, 0xd78610, 0xe53a30, 0xe53b40, 0xe53af0, 0xe4f868, 0xe4bac0, 0xd78668, 0xe4bab0, 0xe4f830, 0xe53a48, 0xe53ab0, 0xe53a90, 0xe53aa8, 0xe53aa0, 0xe53ab8, 0xe53a80, 0xe53a88, 0xe53a70, 0xe53a50, 0xe53ad0, 0xe53ac0, 0xe53a58, 0xd78620, 0xe49a40, 0xe49a20, 0xe4ba88, 0xe4f880, 0xe4ba68, 0xe4ba58, 0xe4f840, 0xe49a00, 0xe4f850, 0xe4bad8, 0xe49a48, 0xe53b10, 0xe53af8, 0xe53b48, 0xe49a10, 0xe53b38, 0xe53b50, 0xe53b58, 0xe4ba80, 0x3c08d38, 0xe5a660, 0xe5a668, 0xe5a658, 0xe5a680, 0xe5a688, 0xe5a678, 0xe5a670, 0xe5a650, 0xe5a648, 0x3c08d98, 0x3c0a628, 0x1d05950, 0xe4f8d8, 0xe4ba60, 0xe53a60, 0xe53a98, 0xe53ad8, 0xe53a78, 0xe4ba70, 0xd78630, 0xe49a08, 0xe49a18, 0xe53a68, 0xe499f8, 0xe4baa0, 0xe4bab8, 0xe4f848, 0xe53b78, 0xe49a28, 0xe49a30, 0xd78640, 0xd76880, 0xe53b88, 0xe53b98]
def map_image(path):
    data = path.read_bytes()
    if hashlib.sha256(data).hexdigest() != DIGEST:
        raise ValueError("Store MP original hash mismatch")
    nt, = struct.unpack_from("<I", data, 60)
    count, = struct.unpack_from("<H", data, nt+6)
    opt, = struct.unpack_from("<H", data, nt+20)
    size, = struct.unpack_from("<I", data, nt+0x50)
    mem = mmap.mmap(-1, size, prot=mmap.PROT_READ|mmap.PROT_WRITE|mmap.PROT_EXEC)
    for i in range(count):
        _, va, rs, raw = struct.unpack_from("<4I", data, nt+24+opt+40*i+8)
        mem[va:va+rs] = data[raw:raw+rs]
    return mem
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so = pathlib.Path(tmp)/"cg.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-Wall","-Wextra","-Werror","-shared","-fPIC",
                        str(ROOT/"tests/store_cgame_registration_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so))
        lib.CgRegistrationMock.argtypes=[C.c_uint]
        lib.CgRegistrationMock.restype=C.c_void_p
        lib.CgRegistrationSize.restype=C.c_uint
        lib.CgRegistrationRun.argtypes=[C.c_void_p,C.c_uint,C.c_void_p]
        lib.CgRegistrationRun.restype=None
        lib.CgRegistrationGlobals.argtypes=[C.c_void_p]
        lib.CgRegistrationGlobals.restype=C.c_uint
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for index,rva in enumerate(TARGETS):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.CgRegistrationMock(index))+b"\xff\xe0"
            old=C.create_string_buffer(lib.CgRegistrationSize())
            new=C.create_string_buffer(lib.CgRegistrationSize())
            globals_=(C.c_uint64*len(GLOBALS))()
            for seed in range(256):
                for rva in GLOBALS:
                    struct.pack_into("<Q",mem,rva,0)
                lib.CgRegistrationRun(base,seed,old)
                lib.CgRegistrationRun(None,seed,new)
                count,=struct.unpack_from("<Q",old.raw)
                assert count==216,(seed,count)
                if old.raw!=new.raw:
                    pos=next(i for i,(x,y) in enumerate(zip(old.raw,new.raw)) if x!=y)
                    idx=(pos-8)//96
                    raise AssertionError((seed,idx,struct.unpack_from("<12Q",old.raw,8+96*idx),struct.unpack_from("<12Q",new.raw,8+96*idx)))
                assert lib.CgRegistrationGlobals(globals_)==125
                assert list(globals_)==[struct.unpack_from("<Q",mem,rva)[0] for rva in GLOBALS],seed
            print("256 complete MP executions; 216 ordered events and 125 globals per case")
if __name__=="__main__":
    main()
