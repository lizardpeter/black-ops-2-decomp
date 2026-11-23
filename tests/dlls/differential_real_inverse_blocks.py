#!/usr/bin/env python3
"""Real BO2 frame's 65 inverse transforms: original x64 versus native C++.

Executes the pinned original FUN_1800252C0 on the first *real*
la_pip_seq_6.bik keyframe. Intercepts each original FUN_18002F2E0 (8x8)
and FUN_18002F630 (16x16 doubled) entry and return, captures the actual
64 coefficients, original indexed BINKCONS matrix, output tile before/after,
and compares with the independently built native C++ source for both the
direct-table and original-caller indexed implementations.

Includes 29 original 8x8 + 36 original doubled 16x16 blocks. Verifies
all pixels for every block and the complete original Y-plane SHA256.
This is still an ORIGINAL full-frame coordinator and does NOT claim
a completely reconstructed source frame decoder.
"""
import ctypes
import hashlib
import struct
import sys
from collections import Counter
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
sys.path.insert(0,str(ROOT/"tools/dlls"))
from harness import Image,Machine,HEAP  # noqa: E402
from probe_original_bik_frame import (  # noqa: E402
    PIN_DLL,PIN_VIDEO,PACKET,SCRATCH_POINTERS,SCRATCH_FIRST,
    SCRATCH_SPACING,STATS,le32,
)
from unicorn import x86_const as X  # noqa: E402

Y_SHA="347b7e90cad7c4cc5d0e3e9b0541813b3a49605827aa72ec59c662ff1f24edaf"
TRANSFORMS={
    0x2F2E0:(0x26DC5,8,"bink_video_inverse8x8",
             "bink_video_inverse8x8_indexed"),
    0x2F630:(0x264EA,16,"bink_video_inverse8x8_2x",
             "bink_video_inverse8x8_2x_indexed"),
}
INTRA_TABLE_BASE=0x5B120
Y_SIZE=256*256

def main():
    orig=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/"originals/dlls/binkw64.dll"
    video=Path(sys.argv[2]) if len(sys.argv)>2 else ROOT/"originals/bik/la_pip_seq_6.bik"
    library=Path(sys.argv[3]) if len(sys.argv)>3 else ROOT/"build/bink_real_intra_transforms.so"
    if hashlib.sha256(orig.read_bytes()).hexdigest()!=PIN_DLL:
        raise SystemExit("Wrong original Store Bink DLL")
    if hashlib.sha256(video.read_bytes()).hexdigest()!=PIN_VIDEO:
        raise SystemExit("Wrong original BO2 video")
    native=ctypes.CDLL(str(library.resolve()))
    for _,(_,_,name,indexed) in TRANSFORMS.items():
        ordinary=getattr(native,name)
        ordinary.argtypes=(ctypes.POINTER(ctypes.c_uint8),ctypes.c_int32,
                           ctypes.POINTER(ctypes.c_int16),
                           ctypes.POINTER(ctypes.c_int32))
        ordinary.restype=None
        wrapper=getattr(native,indexed)
        wrapper.argtypes=(ctypes.POINTER(ctypes.c_uint8),ctypes.c_int32,
                          ctypes.POINTER(ctypes.c_int16),ctypes.c_uint32)
        wrapper.restype=None

    content=video.read_bytes()
    start=le32(content,44)&~1
    end=le32(content,48)&~1
    packet=content[start:end]
    partition=le32(packet,0)
    assert len(packet)==1676 and partition==1372
    image=Image(orig)
    machine=Machine(image)
    pending=[]
    errors=[]
    verified=Counter()

    def tile(mu,pointer,size):
        return b"".join(bytes(mu.mem_read(pointer+row*256,size))
                        for row in range(size))

    def entering(rva):
        def f(mu):
            sp=mu.reg_read(X.UC_X86_REG_RSP)
            retaddr=struct.unpack("<Q",bytes(mu.mem_read(sp,8)))[0]
            expected_return,size,name,indexed=TRANSFORMS[rva]
            if retaddr!=image.base+expected_return:
                errors.append(f"Unexpected transform return {rva:#x} -> {retaddr:#x}")
                return
            dest=mu.reg_read(X.UC_X86_REG_RCX)
            stride=mu.reg_read(X.UC_X86_REG_RDX)&0xFFFFFFFF
            coeff=mu.reg_read(X.UC_X86_REG_R8)
            table=mu.reg_read(X.UC_X86_REG_R9)
            idx_rva=table-image.base-INTRA_TABLE_BASE
            if idx_rva<0 or idx_rva%256 or idx_rva//256>=16:
                errors.append(f"Transform {rva:#x} non-intra matrix ptr {table:#x}")
                return
            if stride!=256 or dest<HEAP or dest+(size-1)*stride+size>HEAP+Y_SIZE:
                errors.append(f"Bad tile pointer/stride at {rva:#x} dst={dest:#x}")
                return
            pending.append({
                "rva":rva,"return":retaddr,"dest":dest,
                "before":tile(mu,dest,size),
                "coeff":bytes(mu.mem_read(coeff,128)),
                "matrix":bytes(mu.mem_read(table,256)),
                "index":idx_rva//256,"size":size,
            })
        return f

    def returning(rva):
        def f(mu):
            if not pending:
                errors.append(f"Transform return {rva:#x} without entry")
                return
            recorded=pending.pop()
            if rva!=recorded["rva"]:
                errors.append("Transform nesting mismatch")
                return
            size=recorded["size"]
            observed=tile(mu,recorded["dest"],size)
            quant=(ctypes.c_int32*64).from_buffer_copy(recorded["matrix"])
            coeff=(ctypes.c_int16*64).from_buffer_copy(recorded["coeff"])
            orig_rva=recorded["rva"]
            _,_,name,indexed=TRANSFORMS[orig_rva]
            for fn_name in (name,indexed):
                output=(ctypes.c_uint8*(size*size)).from_buffer_copy(recorded["before"])
                if fn_name==indexed:
                    getattr(native,fn_name)(output,size,coeff,recorded["index"])
                else:
                    getattr(native,fn_name)(output,size,coeff,quant)
                actual=bytes(output)
                if actual!=observed:
                    bad=next(i for i,(a,b) in enumerate(zip(actual,observed)) if a!=b)
                    errors.append(
                        f"Original vs native real frame {fn_name} tile={recorded['dest']-HEAP}, "
                        f"index={recorded['index']}, byte={bad}: "
                        f"original={observed[bad]}, C++={actual[bad]}")
                    return
            verified[size]+=1
        return f

    hooks={}
    for rva,(return_rva,_,_,_) in TRANSFORMS.items():
        hooks[image.base+rva]=entering(rva)
        hooks[image.base+return_rva]=returning(rva)

    scratch=[SCRATCH_FIRST+i*SCRATCH_SPACING for i in range(9)]
    def setup(w):
        w.write(PACKET,packet)
        w.write(SCRATCH_POINTERS,struct.pack("<9Q",*scratch))
        w.u32(STATS,0)
    ret,bufs,_,_=machine.call(
        image.base+0x252C0,
        [bytes([0xB7])*Y_SIZE,bytes([0x80])*Y_SIZE,256,256,256,
         PACKET+4,PACKET+partition,0,STATS,0,1,SCRATCH_POINTERS,0,STATS],
        ret="uint64",setup=setup,at=hooks,limit=15_000_000)
    if isinstance(ret,str) or ret!=PACKET+partition:
        raise AssertionError(f"Original full-frame transform probe failed: {ret}")
    if pending or errors:
        raise AssertionError(f"Real transform differential errors: {errors[:8]}, "
                             f"unreturned calls={len(pending)}")
    original_y=hashlib.sha256(bufs[0]).hexdigest()
    if original_y!=Y_SHA:
        raise AssertionError(f"Original Y output changed: {original_y}")
    if dict(verified)!={16:36,8:29}:
        raise AssertionError(f"Unexpected real frame inverse transform counts {dict(verified)}")
    print("REAL BO2 original-vs-native inverse transform differential PASS: "
          "36 16x16 2x blocks + 29 8x8 blocks; both direct-table and "
          f"original-caller indexed C++ variants matched all {36*256+29*64} pixels; "
          "full original Y SHA256",Y_SHA)

if __name__=="__main__":main()
