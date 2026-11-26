#!/usr/bin/env python3
"""Differential replay of original Store dvar latched and float setter bodies."""
import ctypes as C
import itertools
import pathlib
import struct
import subprocess
import sys
import tempfile
from verify_store_cgame_registration import ROOT, map_image

def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"setters.so"
        subprocess.run(["g++","-std=c++17","-O2","-Wall","-Wextra","-Werror","-shared","-fPIC",
                        str(ROOT/"tests/store_dvar_setter_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so))
        lib.CoreSetterMock.argtypes=[C.c_uint]
        lib.CoreSetterMock.restype=C.c_void_p
        lib.CoreSetterPool.restype=C.c_void_p
        lib.CoreSetterSize.restype=C.c_uint
        lib.CoreLatchRun.argtypes=[C.c_void_p,C.c_int,*([C.c_uint]*5),C.c_void_p]
        lib.CoreFloatRun.argtypes=[C.c_void_p,C.c_int,C.c_uint,C.c_uint,C.c_int,C.c_void_p]
        lib.CoreEqualRun.argtypes=[C.c_void_p,C.c_int,C.c_void_p,C.c_void_p]
        lib.CoreEqualRun.restype=C.c_int
        lib.CoreEqualStringRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint]
        lib.CoreEqualStringRun.restype=C.c_int
        old=C.create_string_buffer(lib.CoreSetterSize())
        new=C.create_string_buffer(lib.CoreSetterSize())
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x50abe0,0x465c70,0x46b300,0x471ec0]):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.CoreSetterMock(i))+b"\xff\xe0"
            struct.pack_into("<Q",mem,0x2275610,lib.CoreSetterPool())
            count=0
            for kind in [*range(13),-1,99]:
                if kind==8: continue
                args=(kind,0,0,0,0,0)
                lib.CoreLatchRun(base,*args,old)
                lib.CoreLatchRun(None,*args,new)
                assert old.raw==new.raw,(args,old.raw.hex(),new.raw.hex())
                count+=1
            for c,l,r,n,idx in itertools.product(range(9),range(9),range(9),range(1,9),[0,1]):
                args=(8,c,l,r,n,idx)
                lib.CoreLatchRun(base,*args,old)
                lib.CoreLatchRun(None,*args,new)
                assert old.raw==new.raw,(args,old.raw.hex(),new.raw.hex())
                count+=1
            print(f"{count} original SetLatched comparisons; all numeric lanes and string alias ownership paths")
            count=0
            for kind,named,bits,source in itertools.product(range(13),range(4),
                    [0,0x80000000,0x3e4ccccd,0x3f800000,0x40000000,0xc0800000,0x42a00000,0x7f7fffff,1],range(4)):
                args=(kind,named,bits,source)
                lib.CoreFloatRun(base,*args,old)
                lib.CoreFloatRun(None,*args,new)
                assert old.raw==new.raw,(args,old.raw.hex(),new.raw.hex())
                count+=1
            print(f"{count} original SetFloat comparisons; type, null/name, source, float bits and formatting call")
            count=0
            words=[0,1,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc00000,0x7f800001,0x12345678,0xffffffff]
            for kind,lane,a,b in itertools.product([*range(8),9,10,11,12,-1,99],range(4),words,words):
                x=(C.c_uint32*4)(0x3f800000,0x3f800000,0x3f800000,0x3f800000)
                y=(C.c_uint32*4)(*x)
                x[lane]=a;y[lane]=b
                old_eq=lib.CoreEqualRun(base,kind,x,y)
                new_eq=lib.CoreEqualRun(None,kind,x,y)
                assert old_eq==new_eq,(kind,lane,hex(a),hex(b),old_eq,new_eq)
                count+=1
            for a,b in itertools.product(range(1,9),repeat=2):
                assert lib.CoreEqualStringRun(base,a,b)==lib.CoreEqualStringRun(None,a,b),(a,b)
                count+=1
            print(f"{count} original ValuesEqual comparisons; signed zero, NaN, infinity, lane counts and strings")
if __name__=="__main__":
    main()
