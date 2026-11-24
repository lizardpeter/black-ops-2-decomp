#!/usr/bin/env python3
"""Sequential original vs reconstructed BIKi video decoding, all three planes.

Each implementation owns its own alternating destination/reference buffers.
Only the compressed packets are shared; no original decoded pixels or
original plane cursors are fed into the reconstructed decoder. Fail closed
on the first unsupported mode, incorrect pixel, cursor or mode histogram.
The original x64 binary is an independent Unicorn oracle, not a native
decoder dependency. This is a pinned, video-only fixture regression.
"""
import argparse
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
from fetch_bik_fixtures import FIXTURES
from probe_original_bik_frame import (
    PIN_DLL,PACKET,SCRATCH_POINTERS,SCRATCH_FIRST,SCRATCH_SPACING,
)
from unicorn import x86_const as X

# Keep stats beyond all nine scratch regions; the historical address
# overlapped packets larger than 60 KiB.
STATS=SCRATCH_FIRST+9*SCRATCH_SPACING

def sha(data):return hashlib.sha256(data).hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("dll",type=Path)
    p.add_argument("video",type=Path)
    p.add_argument("library",type=Path)
    p.add_argument("--frames",type=int,default=None)
    p.add_argument("--report",type=Path,default=ROOT/"build/bink-sequence-report.json")
    a=p.parse_args()
    assert sha(a.dll.read_bytes())==PIN_DLL
    data=a.video.read_bytes()
    pin=FIXTURES[a.video.name]
    assert sha(data)==pin and data[:4]==b"BIKi"
    count,=struct.unpack_from("<I",data,8)
    width,height=struct.unpack_from("<II",data,20)
    assert count>0 and 16<=width<=1280 and 16<=height<=720
    assert struct.unpack_from("<II",data,36)==(0,0) # no flags/audio
    if a.frames is None:a.frames=count
    assert 1<=a.frames<=count
    offsets=struct.unpack_from(f"<{count+1}I",data,44)
    assert (offsets[-1]&~1)==len(data)
    img=Image(a.dll)
    machine=Machine(img,large_heap=(width*height>400000))
    lib=ctypes.CDLL(str(a.library.resolve()))
    byteptr=ctypes.POINTER(ctypes.c_uint8)
    fn=lib.bink_research_decode_video_plane
    fn.argtypes=(byteptr,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_uint32,
                 ctypes.c_uint32,ctypes.c_uint32,byteptr,ctypes.c_uint32,
                 byteptr,ctypes.POINTER(Trace))
    fn.restype=ctypes.c_int
    # Original BinkGetFrameBuffersInfo rounds full and ceil-half planes to 8.
    align8=lambda x:(x+7)&~7
    dims=((align8(width),align8(height)),
          (align8((width+1)//2),align8((height+1)//2)),
          (align8((width+1)//2),align8((height+1)//2)))
    # Reproduce alternating frame buffers, retaining older pixels in
    # blocks skipped by original boundary guards. Identical initial states
    # in independent original and native storage, not oracle-fed prediction.
    original=[[bytes([0xB7 if slot==0 else 0x80])*(w*h) for w,h in dims]
              for slot in range(2)]
    native=[row.copy() for row in original]
    scratch=[SCRATCH_FIRST+i*SCRATCH_SPACING for i in range(9)]
    report={"fixture":a.video.name,"fixture_sha256":pin,"requested_frames":a.frames,
            "visible_dimensions":[width,height],"plane_dimensions":dims,
            "complete_matching_frames":0,"matched_planes":0,"matched_bytes":0,
            "frames":[]}
    a.report.parent.mkdir(parents=True,exist_ok=True)
    def save():a.report.write_text(json.dumps(report,indent=2)+"\n")
    # The raw planar video is emitted only for fully matched frames.
    # It is a generated CI artifact, never committed as game media.
    video_out=a.report.parent/"native-bink-sequence.yuv"
    with video_out.open("wb") as decoded:
        for frame in range(a.frames):
            start,end=(offsets[frame]&~1,offsets[frame+1]&~1)
            assert 44+4*(count+1)<=start<end<=len(data)
            packet=data[start:end]
            assert len(packet)<=SCRATCH_POINTERS-PACKET
            split,=struct.unpack_from("<I",packet)
            assert 4<split<=len(packet) and split%4==0 and len(packet)%4==0
            raw=(ctypes.c_uint8*len(packet)).from_buffer_copy(packet)
            dst=frame%2
            ref=1-dst
            op=np=4
            frame_report={"index":frame,"keyframe":bool(offsets[frame]&1),
                          "packet_bytes":len(packet),"planes":[]}
            report["frames"].append(frame_report)
            for i,(w,h) in enumerate(dims):
                if i==1:op=np=split
                # Match the verified first-frame luma oracle. Later Y
                # frames use packet-end, as in the tested frame-1 probe.
                limit=split if frame==0 and i==0 else len(packet)
                modes,submodes=Counter(),Counter()
                def mode(mu):
                    modes[mu.reg_read(X.UC_X86_REG_RAX)&0xffffffff]+=1
                def submode(mu):
                    submodes[bytes(mu.mem_read(mu.reg_read(X.UC_X86_REG_RAX),1))[0]]+=1
                def setup(w):
                    w.write(PACKET,packet)
                    w.write(SCRATCH_POINTERS,struct.pack("<9Q",*scratch))
                    w.u32(STATS,0)
                returned,bufs,_,_=machine.call(
                    img.base+0x252C0,
                    [original[dst][i],original[ref][i],w,h,w,
                     PACKET+op,PACKET+limit,0,STATS,0,1 if i==0 else 2,
                     SCRATCH_POINTERS,0,STATS],
                    ret="uint64",setup=setup,
                    at={img.base+0x25A32:mode,img.base+0x25BD5:submode},
                    limit=max(15_000_000,w*h*200))
                if isinstance(returned,str) or not PACKET+op<returned<=PACKET+limit:
                    report["failure"]={"frame":frame,"plane":i,"original_return":str(returned)}
                    save()
                    raise AssertionError(report["failure"])
                expected=bufs[0]
                op=returned-PACKET
                out=(ctypes.c_uint8*(w*h)).from_buffer_copy(native[dst][i])
                prior=(ctypes.c_uint8*(w*h)).from_buffer_copy(native[ref][i])
                trace=Trace()
                status=fn(raw,len(packet),np,limit,w,h,out,w,prior,ctypes.byref(trace))
                actual=bytes(out)
                np=4*trace.input_words
                # Avoid scanning megabytes in Python on the matching path.
                differences=[] if expected==actual else [k for k,(v,w) in enumerate(zip(expected,actual)) if v!=w]
                modes_match=(list(trace.type_counts)==[modes[k] for k in range(10)]
                         and list(trace.submode_counts)==[submodes[k] for k in range(10)])
                record={"plane":i,"width":w,"height":h,"status":status,
                        "original_sha256":sha(expected),"native_sha256":sha(actual),
                        "original_next":op,"native_next":np,
                        "original_modes":dict(modes),"original_submodes":dict(submodes),
                        "native_modes":list(trace.type_counts),
                        "native_submodes":list(trace.submode_counts),
                        "mode_counts_match":modes_match,"differences":len(differences),
                        "first_differences":[[k%w,k//w,expected[k],actual[k]]
                                             for k in differences[:12]],
                        "native_position":[trace.error_x,trace.error_y,trace.mode,trace.submode]}
                frame_report["planes"].append(record)
                if (status or differences or op!=np or not modes_match
                        or bytes(prior)!=native[ref][i] or bufs[1]!=original[ref][i]):
                    report["failure"]={"frame":frame,**record}
                    header=f"P5\n{w} {h}\n255\n".encode()
                    (a.report.parent/"sequence-failed-original.pgm").write_bytes(header+expected)
                    (a.report.parent/"sequence-failed-native.pgm").write_bytes(header+actual)
                    save()
                    raise AssertionError("First sequence mismatch: "+json.dumps(report["failure"]))
                original[dst][i]=expected
                native[dst][i]=actual
                report["matched_planes"]+=1
                report["matched_bytes"]+=len(actual)
                if frame==0 and i==0 and pin==PIN_VIDEO:assert sha(actual)==PIN_Y
            assert op==np==len(packet)
            for plane in native[dst]:decoded.write(plane)
            report["complete_matching_frames"]+=1
            save()
            print(f"SEQUENCE PASS frame={frame} bytes={report['matched_bytes']} "
                  f"Y={frame_report['planes'][0]['native_sha256']}",flush=True)
    print(f"FULL SEQUENCE PASS: {report['complete_matching_frames']} frames, "
          f"{report['matched_planes']} planes, {report['matched_bytes']} bytes")

if __name__=="__main__":main()
