#!/usr/bin/env python3
"""Linux x86-64 isolated replay of pinned Store built-in texture constructors.
Only setup/upload callees are mocked; original bytes are never saved or executed as a game.
"""
import ctypes as C
import hashlib
import mmap
import pathlib
import random
import struct
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
BUILDS = {
    'mp': ('cf80b8058bce7e00e4c61ff3d673308c1fcd8061f18fe66710803538eab4bc01', 0x76f650, 0x76f6e0, 0x76f7c0, 0x7776a0, 0x777440),
    'zm': ('7152ebe1c892ea8b1ea7a73811cb82bcdc22088816f5c731a6637d5461bb7416', 0x76d190, 0x76d220, 0x76d300, 0x7751e0, 0x774f80),
    'sp': ('0404f79cb36394b14cfc3321b7aa126a08518531564fa042fc14b82debe45c89', 0x61bc10, 0x61bca0, 0x61bd80, 0x61cf90, 0x61cd30),
}
class PE:
    def __init__(self, path, digest):
        self.b = path.read_bytes()
        assert hashlib.sha256(self.b).hexdigest() == digest, path
        h, = struct.unpack_from('<I', self.b, 60)
        n, = struct.unpack_from('<H', self.b, h+6)
        opt, = struct.unpack_from('<H', self.b, h+20)
        self.sections = {}
        for i in range(n):
            o = h+24+opt+40*i
            name = self.b[o:o+8].rstrip(b'\0').decode()
            self.sections[name] = struct.unpack_from('<4I',self.b,o+8)
        vs,va,rs,raw = self.sections['.pdata']
        self.rows = {a:e for a,e,u in struct.iter_unpack('<III', self.b[raw:raw+min(vs,rs)//12*12]) if a and e>a}
    def read(self, a, size):
        for vs,va,rs,raw in self.sections.values():
            if va <= a and a+size <= va+rs:
                return self.b[raw+a-va:raw+a-va+size]
        raise ValueError(hex(a))

def replay(lib, folder, mode, spec, reference):
    digest, gen, defaults, intensities, setup, upload = spec
    pe = PE(folder / ('t6'+mode+'.exe'),digest)
    entries = [gen] + [defaults+i*0x20 for i in range(7)] + [intensities+i*0x70 for i in range(23)]
    size=0x3000
    mem=mmap.mmap(-1,size,prot=mmap.PROT_READ|mmap.PROT_WRITE|mmap.PROT_EXEC)
    base=C.addressof(C.c_char.from_buffer(mem))
    slots={a:i*0x100 for i,a in enumerate(entries)}
    for a,stub in [(setup,'ReplaySetup'),(upload,'ReplayUpload')]:
        slots[a]=0x2000+(0x20 if a==upload else 0)
        addr=C.cast(getattr(lib,stub),C.c_void_p).value
        mem[slots[a]:slots[a]+12]=b'\x48\xb8'+struct.pack('<Q',addr)+b'\xff\xe0'
    normalized=[]
    for i,a in enumerate(entries):
        code=bytearray(pe.read(a,pe.rows[a]-a)); norm=bytearray(code)
        assert len(code)<0x100 and code[-1]==0xc3
        found=[]
        for off in range(len(code)-4):
            if code[off]!=0xe8: continue
            dst=a+off+5+struct.unpack_from('<i',code,off+1)[0]
            if dst not in (gen,setup,upload): continue
            found.append(dst)
            struct.pack_into('<i',code,off+1,slots[dst]-slots[a]-off-5)
            norm[off+1:off+5]=bytes(4)
        assert found==([gen] if 1<=i<=7 else [setup,upload]), (mode,hex(a),found)
        normalized.append(bytes(norm))
        mem[slots[a]:slots[a]+len(code)]=code
    if reference is not None: assert reference==normalized, mode
    rng=random.Random(0xB02)
    count=0
    for i,a in enumerate(entries):
        for rgba in ([0,0xffffffff,0x01020304]+[rng.getrandbits(32) for _ in range(1021)] if not i else [0]):
            native=C.create_string_buffer(60); original=C.create_string_buffer(60)
            lib.CaptureNative(i,rgba,native)
            lib.CaptureOriginal(base+slots[a],i,rgba,original)
            assert native.raw==original.raw, (mode,i,rgba,native.raw.hex(),original.raw.hex())
            assert struct.unpack_from('<I',native.raw,56)[0]==0
            count+=1
    mem.close()
    print(f'{mode}: {len(entries)} bodies, {count} comparisons passed')
    return normalized

def main():
    folder=pathlib.Path(sys.argv[1])
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/'probe.so'
        subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-DT6_STORE','-I'+str(ROOT/'include'),str(ROOT/'src/gfx_d3d/r_image_load_obj.cpp'),str(ROOT/'tests/store_image_builtin_probe.cpp'),'-o',str(so)],check=True)
        lib=C.CDLL(str(so)); lib.CaptureNative.argtypes=[C.c_uint,C.c_uint,C.c_void_p]; lib.CaptureNative.restype=None
        lib.CaptureOriginal.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p]; lib.CaptureOriginal.restype=None
        ref=None
        for mode,spec in BUILDS.items(): ref=replay(lib,folder,mode,spec,ref)
if __name__=='__main__': main()
