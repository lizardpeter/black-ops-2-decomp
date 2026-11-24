#!/usr/bin/env python3
"""Compare reconstructed source vs original Bink for all three planes.

The original 0x274A0 coordinator selects Y at packet+4, then selects
first chroma at packet+u32(packet), and second chroma at the returned
word pointer of original 0x252C0. Both chroma planes have half the
width/height of Y on this fixture. Chroma is decoded sequentially by
the source using its OWN returned cursor, not the original's cursor.

This is a restricted first-keyframe test, not a full Bink player.
"""
import ctypes
import hashlib
import json
import struct
import sys
from collections import Counter
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
sys.path.insert(0,str(ROOT/"tools/dlls"))
from harness import Image,Machine
from experimental_first_luma import Trace,PIN_VIDEO,PIN_Y
from probe_original_bik_frame import (
    PIN_DLL,PACKET,SCRATCH_POINTERS,SCRATCH_FIRST,SCRATCH_SPACING,STATS,
)
from unicorn import x86_const as X

def main():
    dll,video,source=map(Path,sys.argv[1:4])
    assert hashlib.sha256(dll.read_bytes()).hexdigest()==PIN_DLL
    data=video.read_bytes()
    assert hashlib.sha256(data).hexdigest()==PIN_VIDEO
    assert data[:4]==b"BIKi"
    first,end=(struct.unpack_from("<I",data,i)[0]&~1 for i in (44,48))
    packet=data[first:end]
    split=struct.unpack_from("<I",packet)[0]
    assert len(packet)==1676 and split==1372
    img=Image(dll)
    m=Machine(img)
    native=ctypes.CDLL(str(source.resolve()))
    fn=native.bink_research_decode_keyframe_plane
    fn.argtypes=(ctypes.POINTER(ctypes.c_uint8),ctypes.c_uint32,
                 ctypes.c_uint32,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_uint32,
                 ctypes.POINTER(ctypes.c_uint8),ctypes.c_uint32,ctypes.POINTER(Trace))
    fn.restype=ctypes.c_int
    raw=(ctypes.c_uint8*len(packet)).from_buffer_copy(packet)
    start_original=start_native=4
    planes=[]
    mismatched=False
    for plane,side,limit in (("Y",256,split),("C1",128,len(packet)),("C2",128,len(packet))):
        # First chroma starts at the frame header's partition offset;
        # the second chroma starts at the previous plane's return pointer.
        if plane=="C1":
            start_original=start_native=split
        modes=Counter()
        submodes=Counter()
        def on_mode(mu):
            modes[mu.reg_read(X.UC_X86_REG_RAX)&0xffffffff]+=1
        def on_submode(mu):
            submodes[bytes(mu.mem_read(mu.reg_read(X.UC_X86_REG_RAX),1))[0]]+=1
        scratch=[SCRATCH_FIRST+i*SCRATCH_SPACING for i in range(9)]
        def setup(w):
            w.write(PACKET,packet)
            w.write(SCRATCH_POINTERS,struct.pack("<9Q",*scratch))
            w.u32(STATS,0)
        result,bufs,_,_=m.call(
            img.base+0x252C0,
            [bytes([0xB7])*(side*side),bytes([0x80])*(side*side),
             side,side,side,PACKET+start_original,PACKET+limit,0,
             STATS,0,1 if plane=="Y" else 2,SCRATCH_POINTERS,0,STATS],
            ret="uint64",setup=setup,
            at={img.base+0x25A32:on_mode,img.base+0x25BD5:on_submode},
            limit=15_000_000)
        if isinstance(result,str) or not PACKET+start_original<result<=PACKET+limit:
            raise AssertionError(f"Original {plane} decode fault: {result}")
        expected=bufs[0]
        orig_sha=hashlib.sha256(expected).hexdigest()
        if plane=="Y": assert orig_sha==PIN_Y
        original_next=result-PACKET
        print(f"Original {plane} {side}x{side} start={start_original} "
              f"next={original_next} SHA={orig_sha} "
              f"modes={dict(modes)} submodes={dict(submodes)}",flush=True)
        build=ROOT/"build"
        build.mkdir(exist_ok=True)
        header=f"P5\n{side} {side}\n255\n".encode()
        (build/f"original-bink-frame0-{plane}.pgm").write_bytes(header+expected)
        out=(ctypes.c_uint8*(side*side))(*([0xB7]*(side*side)))
        trace=Trace()
        if mismatched and plane=="C2":
            # Still record original second chroma, but do not pretend a
            # failed native first-chroma cursor is a valid input.
            print("Native C2 skipped because earlier plane did not match.")
            break
        status=fn(raw,len(packet),start_native,limit,side,side,out,side,ctypes.byref(trace))
        actual=bytes(out)
        native_next=4*trace.input_words
        actual_sha=hashlib.sha256(actual).hexdigest()
        diffs=sum(a!=b for a,b in zip(actual,expected))
        print(f"Native {plane}: status={status}, position={trace.error_x},{trace.error_y}, "
              f"mode={trace.mode} submode={trace.submode}, next={native_next}, "
              f"SHA={actual_sha} differences={diffs}, modes={list(trace.type_counts)}, "
              f"submodes={list(trace.submode_counts)}",flush=True)
        modes_match=(list(trace.type_counts)==[modes[i] for i in range(10)]
                     and list(trace.submode_counts)==[submodes[i] for i in range(10)])
        good=status==0 and diffs==0 and native_next==original_next and modes_match
        planes.append(dict(plane=plane,side=side,original_sha=orig_sha,native_sha=actual_sha,
                           original_next=original_next,native_next=native_next,
                           mismatched_pixels=diffs,status=status,mode_counts_match=modes_match,passed=good))
        if good:
            (build/f"native-bink-frame0-{plane}.pgm").write_bytes(header+actual)
        mismatched=mismatched or not good
        start_original=original_next
        start_native=native_next
    (ROOT/"build/bink-first-color-report.json").write_text(json.dumps(planes,indent=2)+"\n")
    if mismatched or len(planes)!=3:
        raise AssertionError("Not yet a byte-exact complete source color keyframe")
    print("SOURCE COLOR KEYFRAME PASS: all 98304 Y/C1/C2 bytes and plane cursors matched")

if __name__=="__main__":main()
