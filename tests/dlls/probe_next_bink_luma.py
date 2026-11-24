#!/usr/bin/env python3
"""Record the next original Bink luma frame using verified native frame 0.

This is an original-only diagnostic for the dependent-frame reconstruction:
the source-generated frame-0 Y bytes are used as the previous image, and the
original x64 plane decoder executes frame 1. This establishes the golden
output and original block-mode distribution; it does not claim source
decoding of the dependent frame.
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
from experimental_first_luma import PIN_VIDEO,PIN_Y,Trace
from probe_original_bik_frame import (
    PIN_DLL,PACKET,SCRATCH_POINTERS,SCRATCH_FIRST,SCRATCH_SPACING,STATS,
)
from unicorn import x86_const as X

def main():
    dll,video,previous=map(Path,sys.argv[1:4])
    assert hashlib.sha256(dll.read_bytes()).hexdigest()==PIN_DLL
    data=video.read_bytes()
    assert hashlib.sha256(data).hexdigest()==PIN_VIDEO
    header=b"P5\n256 256\n255\n"
    ref=previous.read_bytes()
    assert ref.startswith(header)
    ref=ref[len(header):]
    assert len(ref)==65536 and hashlib.sha256(ref).hexdigest()==PIN_Y
    raw_start,raw_end=struct.unpack_from("<II",data,48)
    start,end=raw_start&~1,raw_end&~1
    assert 0<start<end<=len(data)
    packet=data[start:end]
    split,=struct.unpack_from("<I",packet)
    assert 4<split<=len(packet) and split%4==0
    img=Image(dll)
    machine=Machine(img)
    modes=Counter()
    submodes=Counter()
    def mode(mu):
        modes[mu.reg_read(X.UC_X86_REG_RAX)&0xffffffff]+=1
    def submode(mu):
        submodes[bytes(mu.mem_read(mu.reg_read(X.UC_X86_REG_RAX),1))[0]]+=1
    def setup(w):
        w.write(PACKET,packet)
        w.write(SCRATCH_POINTERS,struct.pack("<9Q",*[
            SCRATCH_FIRST+i*SCRATCH_SPACING for i in range(9)]))
        w.u32(STATS,0)
    # Original full-frame coordinator supplies the whole compressed packet
    # end to the plane core. The luma header defines the next plane offset.
    returned,buffers,_,_=machine.call(
        img.base+0x252C0,
        [bytes([0xB7])*65536,ref,256,256,256,PACKET+4,PACKET+len(packet),
         0,STATS,0,1,SCRATCH_POINTERS,0,STATS],
        ret="uint64",setup=setup,at={img.base+0x25A32:mode,img.base+0x25BD5:submode},
        limit=15_000_000)
    assert not isinstance(returned,str),returned
    assert PACKET+4<returned<=PACKET+len(packet)
    expected=buffers[0]
    report={
        "source":"original x64 Bink core with verified native frame-0 reference",
        "frame":1,"keyframe":bool(raw_start&1),"packet_bytes":len(packet),
        "luma_partition":split,"returned_offset":returned-PACKET,
        "previous_y_sha256":PIN_Y,
        "original_y_sha256":hashlib.sha256(expected).hexdigest(),
        "mode_counts":dict(modes),"submode_counts":dict(submodes),
        "unchanged_destination_bytes":expected.count(0xB7),
    }
    build=ROOT/"build"
    build.mkdir(exist_ok=True)
    (build/"original-bink-frame1-Y.pgm").write_bytes(header+expected)
    (build/"original-bink-frame1-report.json").write_text(json.dumps(report,indent=2)+"\n")
    print("ORIGINAL NEXT FRAME:",json.dumps(report,sort_keys=True))
    if len(sys.argv)>4:
        native=ctypes.CDLL(str(Path(sys.argv[4]).resolve()))
        fn=native.bink_research_decode_video_plane
        byteptr=ctypes.POINTER(ctypes.c_uint8)
        fn.argtypes=(byteptr,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_uint32,
                     ctypes.c_uint32,ctypes.c_uint32,byteptr,ctypes.c_uint32,
                     byteptr,ctypes.POINTER(Trace))
        fn.restype=ctypes.c_int
        raw=(ctypes.c_uint8*len(packet)).from_buffer_copy(packet)
        reference=(ctypes.c_uint8*65536).from_buffer_copy(ref)
        output=(ctypes.c_uint8*65536)(*([0xB7]*65536))
        trace=Trace()
        status=fn(raw,len(packet),4,len(packet),256,256,output,256,
                  reference,ctypes.byref(trace))
        actual=bytes(output)
        mismatches=[i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b]
        counts=list(trace.type_counts)
        subcounts=list(trace.submode_counts)
        report.update(native_status=status,native_y_sha256=hashlib.sha256(actual).hexdigest(),
            native_offset=4*trace.input_words,native_modes=counts,native_submodes=subcounts,
            pixel_differences=len(mismatches),
            first_differences=[(i%256,i//256,expected[i],actual[i]) for i in mismatches[:16]],
            error_position=[trace.error_x,trace.error_y,trace.mode,trace.submode])
        print("NATIVE NEXT FRAME:",json.dumps(report,sort_keys=True),flush=True)
        (build/"original-bink-frame1-report.json").write_text(json.dumps(report,indent=2)+"\n")
        (build/"native-bink-frame1-Y.pgm").write_bytes(header+actual)
        assert status==0 and not mismatches and 4*trace.input_words==returned-PACKET
        assert counts==[modes[i] for i in range(10)] and subcounts==[submodes[i] for i in range(10)]
        assert bytes(reference)==ref
        print("SOURCE DEPENDENT FRAME 1 Y PASS: 65536 exact bytes and mode/cursor state")

if __name__=="__main__":main()
