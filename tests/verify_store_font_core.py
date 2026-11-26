#!/usr/bin/env python3
"""Original Store MP font core differential tests, with external icon/DB boundaries."""
import ctypes as C
import pathlib,random,struct,subprocess,sys,tempfile
from verify_store_cgame_registration import ROOT,map_image
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"font-core.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_font_core_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.CoreMock.argtypes=[C.c_uint];lib.CoreMock.restype=C.c_void_p
        lib.CoreSize.restype=C.c_uint;lib.CoreRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_uint,C.c_void_p];lib.CoreRun.restype=None
        a=C.create_string_buffer(lib.CoreSize());b=C.create_string_buffer(lib.CoreSize());rng=random.Random(0x71a6e0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x8a7dd8,0x212150,0x723450]):
                ptr=lib.CoreMock(i)
                if i==0:struct.pack_into("<Q",mem,rva,ptr)
                else:mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",ptr)+b"\xff\xe0"
            values=[0,0x80000000,0x7fc00000,0x7f800000,0xff800000]+[struct.unpack("<I",struct.pack("<f",i*0.125))[0] for i in range(-32,65)]
            for op in range(5):
                for n in range(8192):
                    seed=rng.randrange(1<<25);bits=rng.choice(values)
                    lib.CoreRun(base,op,seed,bits,a);lib.CoreRun(None,op,seed,bits,b)
                    if a.raw!=b.raw:
                        x=struct.unpack("<"+"Q"*(len(a.raw)//8),a.raw);y=struct.unpack("<"+"Q"*(len(b.raw)//8),b.raw)
                        raise AssertionError((op,n,hex(seed),hex(bits),[(i,hex(v),hex(w)) for i,(v,w) in enumerate(zip(x,y)) if v!=w][:30]))
                print(f"8192 Store core font comparisons for operation {op}",flush=True)
if __name__=="__main__":main()
