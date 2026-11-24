#!/usr/bin/env python3
"""Independent full original FUN_1800274A0 frame-coordinator oracle.

Unlike the plane-by-plane differential, this lets the original x64
coordinator choose its own plane order, pointers, packet splits and
frame slots. Native calls use only the original container header and
compressed packet; the observed original call trace is compared, never
used as native input. Test-only descriptor and scratch allocation;
not a reproduction of BinkOpen, timing or async behavior.
"""
import argparse
import ctypes
import hashlib
import json
import struct
import sys
import urllib.request
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
sys.path.insert(0,str(ROOT/"tests/dlls"))
sys.path.insert(0,str(ROOT/"tools/dlls"))
from harness import Image,Machine,OBJECTS,STACK,STACK_SIZE
from experimental_first_luma import Trace
from fetch_bik_fixtures import FIXTURES,BASE
from probe_original_bik_frame import PIN_DLL,PACKET,SCRATCH_POINTERS
from unicorn import x86_const as X

# Extra alpha assets are isolated from the existing video-only fixture set.
ALPHA={
    "generalstore.bik":"4067bb05d24030a7676adf53e35df3cb0308ed87bddfc3f5c6c2343542e269ce",
    "yemen_mendz.bik":"282f1553eecec45305a6fa4e5dda431831aec7e1e2e51bfa5b65d18d3384dcde",
}
DESCRIPTOR=OBJECTS+0x1000
SIZES=OBJECTS+0x2000
ARENA=0x32000000
COORD=0x274A0
PLANE=0x252C0
RETURNS=(0x277D7,0x27887,0x27946,0x279C4)
ALPHA_FLAG=0x100000

def sha(data):return hashlib.sha256(data).hexdigest()
def round8(n):return (n+7)&~7
def u32(data,pos):return struct.unpack_from("<I",data,pos)[0]
def fetch_alpha(directory):
    directory.mkdir(parents=True,exist_ok=True)
    for name,pin in ALPHA.items():
        dest=directory/name
        if dest.exists() and sha(dest.read_bytes())==pin:continue
        req=urllib.request.Request(BASE+"bo2/DUMP/video/"+name,
                                   headers={"User-Agent":"bo2-bink-coordinator/1"})
        with urllib.request.urlopen(req,timeout=45) as remote:
            data=remote.read(8*1024*1024+1)
        assert len(data)<=8*1024*1024 and sha(data)==pin
        dest.write_bytes(data)

def run(dll,video,library,frames,report_path,expected_sha256=None,lookahead=4,padding_byte=0,pitch_extra=0):
    assert lookahead in (0,4) and 0<=padding_byte<=255
    assert 0<=pitch_extra<=256 and pitch_extra%8==0
    assert sha(dll.read_bytes())==PIN_DLL
    data=video.read_bytes()
    pin=expected_sha256 or {**FIXTURES,**ALPHA}[video.name]
    assert sha(data)==pin and data[:4]==b"BIKi"
    count,width,height,flags=u32(data,8),u32(data,20),u32(data,24),u32(data,36)
    assert u32(data,40)==0 and flags in (0,ALPHA_FLAG)
    alpha=bool(flags&ALPHA_FLAG)
    assert 1<=frames<=count
    yw,yh=round8(width),round8(height)
    cw,ch=round8((width+1)//2),round8((height+1)//2)
    dims=[(yw,yh),(cw,ch),(cw,ch)]+([(yw,yh)] if alpha else [])
    order=[3,0,1,2] if alpha else [0,1,2]
    pitches=[w+pitch_extra for w,h in dims]
    storage=[pitch*h for pitch,(w,h) in zip(pitches,dims)]
    offsets=struct.unpack_from(f"<{count+1}I",data,44)
    img=Image(dll)
    machine=Machine(img,large_heap=True)
    lib=ctypes.CDLL(str(library.resolve()))
    ptr=ctypes.POINTER(ctypes.c_uint8)
    fn=lib.bink_research_decode_video_plane
    fn.argtypes=(ptr,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_uint32,
                 ctypes.c_uint32,ctypes.c_uint32,ptr,ctypes.c_uint32,
                 ptr,ctypes.POINTER(Trace))
    fn.restype=ctypes.c_int
    # Give the original independent plane buffers in an explicit x64 arena.
    # A guard region after every plane is checked for writes by the original.
    original=[]
    for slot in range(2):
        # Distinct sentinel row padding makes unintended writes observable.
        original.append([(bytes([0xB7 if slot==0 else 0x80])*w+
                          bytes([0xD3 if slot==0 else 0xE4])*(pitch-w))*h
                         for pitch,(w,h) in zip(pitches,dims)])
    native=[p.copy() for p in original]
    addresses=[[],[]]
    cursor=ARENA
    guard=bytes([0xA7])*4096
    for slot in range(2):
        for size in storage:
            addresses[slot].append(cursor)
            cursor=(cursor+size+len(guard)+4095)&~4095
    assert cursor<=ARENA+machine.heap_size, 'oracle arena too small for two padded four-plane frames'
    result={"fixture":video.name,"sha256":pin,"visible_dimensions":[width,height],
            "plane_dimensions":dims,"plane_pitches":pitches,"original_coordinator_rva":"0x274A0",
            "lookahead_bytes":lookahead,"padding_byte":padding_byte,
            "complete_matching_frames":0,"matched_planes":0,"matched_bytes":0,"frames":[]}
    report_path.parent.mkdir(parents=True,exist_ok=True)
    def save():report_path.write_text(json.dumps(result,indent=2)+"\n")
    output=report_path.parent/"coordinator-matched.yuv"
    output.unlink(missing_ok=True)
    for frame in range(frames):
        start,end=offsets[frame]&~1,offsets[frame+1]&~1
        assert 44+4*(count+1)<=start<end<=len(data)
        packet=data[start:end]
        assert len(packet)+lookahead<SCRATCH_POINTERS-PACKET and len(packet)%4==0
        native_packet=packet+bytes([padding_byte])*lookahead
        limit_bytes=len(native_packet)
        aend=u32(packet,0) if alpha else 0
        assert not alpha or 4<aend<len(packet)-4 and aend%4==0
        yend=aend+u32(packet,aend)
        assert aend+4<yend<len(packet) and yend%4==0
        dst,ref=frame%2,1-frame%2
        # Original FUN_180016420 stores current index low bit in handle+0x140.
        # BinkDoFramePlane forwards that value as coordinator argument five.
        keyframe=(offsets[frame]&1) if lookahead else 0
        # Original descriptor layout verified by BinkGetFrameBuffersInfo.
        # The slot field names the reference slot; the coordinator selects
        # the opposite destination when that slot is allocated.
        descriptor=bytearray(0xD8)
        struct.pack_into("<6I",descriptor,0,2,yw,yh,cw,ch,ref)
        for s in range(2):
            for p,(w,h) in enumerate(dims):
                pos=0x18+s*0x60+p*0x18
                struct.pack_into("<I",descriptor,pos,1)
                struct.pack_into("<Q",descriptor,pos+8,addresses[s][p])
                struct.pack_into("<I",descriptor,pos+16,pitches[p])
        calls=[]
        def at_plane(mu):
            sp=mu.reg_read(X.UC_X86_REG_RSP)
            stack=bytes(mu.mem_read(sp+0x28,0x50))
            stride,begin,limit=struct.unpack_from("<3Q",stack)
            calls.append({"dst":mu.reg_read(X.UC_X86_REG_RCX),
                          "ref":mu.reg_read(X.UC_X86_REG_RDX),
                          "width":mu.reg_read(X.UC_X86_REG_R8)&0xffffffff,
                          "height":mu.reg_read(X.UC_X86_REG_R9)&0xffffffff,
                          "stride":stride&0xffffffff,"start":begin-PACKET,
                          "limit":limit-PACKET,
                          "keyframe":struct.unpack_from("<I",stack,0x18)[0],
                          "plane_flags":struct.unpack_from("<I",stack,0x40)[0]})
        def at_return(mu):
            calls[-1]["next"]=mu.reg_read(X.UC_X86_REG_RAX)-PACKET
        def setup(writer):
            # Direct scratch writes, no originals supplied to native decode.
            mu=machine.mu
            mu.mem_write(DESCRIPTOR,bytes(descriptor))
            mu.mem_write(SIZES,struct.pack("<9Q",*([16384]*9)))
            mu.mem_write(PACKET,native_packet+bytes(256))
            mu.mem_write(STACK,bytes(STACK_SIZE-0x10000))
            for s in range(2):
                for p in range(len(dims)):
                    mu.mem_write(addresses[s][p],original[s][p]+guard)
        hooks={img.base+PLANE:at_plane}
        hooks.update({img.base+x:at_return for x in RETURNS})
        # The original coordinator increments argument 6 by four before
        # forwarding its exclusive word-end. lookahead=4 models the real
        # BinkDoFramePlane caller (handle+0x13c full packet length, RVA
        # 0x164f9 and 0x1a975), while zero preserves the previous oracle.
        returned,_,_,_=machine.call(img.base+COORD,
            [DESCRIPTOR,0,0,PACKET,keyframe,PACKET+limit_bytes-4,SIZES,flags,flags],
            ret="uint",setup=setup,at=hooks,limit=max(15_000_000,yw*yh*400))
        rec={"index":frame,"original_return":returned,"original_plane_calls":calls,"keyframe":keyframe,
             "alpha_end":aend,"y_end":yend,"planes":[]}
        result["frames"].append(rec)
        save()
        if isinstance(returned,str) or len(calls)!=len(order):
            raise AssertionError(f"original coordinator failure: {returned}, calls={calls}")
        raw=(ctypes.c_uint8*limit_bytes).from_buffer_copy(native_packet)
        pos=4
        for j,p in enumerate(order):
            w,h=dims[p]
            pitch,size=pitches[p],storage[p]
            # Independently derive partition routing from packet headers.
            if p==0:pos=aend+4
            if p==1:pos=yend
            before=native[dst][p]
            previous=native[ref][p]
            out=(ctypes.c_uint8*len(before)).from_buffer_copy(before)
            prior=(ctypes.c_uint8*len(previous)).from_buffer_copy(previous)
            trace=Trace()
            status=fn(raw,limit_bytes,pos,limit_bytes,w,h,out,pitch,prior,ctypes.byref(trace))
            next_pos=4*trace.input_words
            expected=bytes(machine.mu.mem_read(addresses[dst][p],size))
            actual=bytes(out)
            original_guard=bytes(machine.mu.mem_read(addresses[dst][p]+size,len(guard)))
            original_ref=bytes(machine.mu.mem_read(addresses[ref][p],size))
            route=calls[j]
            route_match=(route["dst"]==addresses[dst][p] and route["ref"]==addresses[ref][p]
                         and route["width"]==w and route["height"]==h and route["stride"]==pitch
                         and route["start"]==pos and route["limit"]==limit_bytes
                         and route["keyframe"]==keyframe)
            plane={"plane":p,"status":status,"route_match":route_match,
                   "native_start":pos,"native_next":next_pos,"original_next":route.get("next"),
                   "native_sha256":sha(actual),"original_sha256":sha(expected),
                   "pitch":pitch,"padding_unchanged":all(actual[y*pitch+w:(y+1)*pitch]==before[y*pitch+w:(y+1)*pitch] for y in range(h)),
                   "guard_unchanged":original_guard==guard,"reference_unchanged":
                   bytes(prior)==previous and original_ref==original[ref][p],
                   "native_position":[trace.error_x,trace.error_y,trace.mode,trace.submode]}
            rec["planes"].append(plane)
            save()
            if (status or not route_match or route.get("next")!=next_pos or expected!=actual
                or not plane["guard_unchanged"] or not plane["reference_unchanged"] or not plane["padding_unchanged"]):
                raise AssertionError(f"coordinator differential mismatch frame={frame}: {plane}; "
                                     f"original route={route}, expected dst={addresses[dst][p]:#x} "
                                     f"ref={addresses[ref][p]:#x} dims={w}x{h} pitch={pitch} limit={limit_bytes}")
            original[dst][p]=expected
            native[dst][p]=actual
            result["matched_planes"]+=1
            result["matched_bytes"]+=len(actual)
            pos=next_pos
        with output.open("ab") as decoded:
            for p in range(len(dims)):decoded.write(native[dst][p])
        result["complete_matching_frames"]+=1
        save()
        print(f"COORDINATOR PASS {video.name} frame={frame} planes={len(order)}",flush=True)
    print(f"FULL COORDINATOR PASS: {frames} frames, {result['matched_planes']} planes, {result['matched_bytes']} bytes")

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--fetch-alpha",type=Path)
    p.add_argument("--dll",type=Path)
    p.add_argument("--video",type=Path)
    p.add_argument("--library",type=Path)
    p.add_argument("--frames",type=int,default=16)
    p.add_argument("--lookahead",type=int,choices=(0,4),default=4)
    p.add_argument("--pad-byte",type=lambda v:int(v,0),default=0)
    p.add_argument("--pitch-extra",type=int,default=0)
    p.add_argument("--manifest",type=Path,default=None)
    p.add_argument("--report",type=Path,default=Path("build/coordinator-report.json"))
    a=p.parse_args()
    if a.fetch_alpha is not None:fetch_alpha(a.fetch_alpha)
    else:
        assert a.dll and a.video and a.library
        pin=None
        if a.manifest:
            pins={c["name"]:c["sha256"] for c in json.loads(a.manifest.read_text())["clips"]}
            pin=pins[a.video.name]
        run(a.dll,a.video,a.library,a.frames,a.report,expected_sha256=pin,
            lookahead=a.lookahead,padding_byte=a.pad_byte,pitch_extra=a.pitch_extra)

if __name__=="__main__":main()
