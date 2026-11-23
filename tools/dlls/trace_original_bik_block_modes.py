#!/usr/bin/env python3
"""Trace original Bink frame block modes on a REAL BO2 keyframe.

Hooks original FUN_1800252C0's decoded block-mode dispatch (RVA 0x25A32)
and common tile epilogue (RVA 0x25B17), while executing the original pinned
Store binkw64.dll under Unicorn. Records which of modes 0..9 occur on the
real 256x256 luma keyframe, destinations, a few source/reference bytes
and exact destination bytes before and after for later original-vs-source
block-operation reconstruction.

Uses only ORIGINAL code for this trace. The output is a diagnostic corpus,
not a native Bink frame decoder and not an external DLL replacement.
"""
import argparse
from collections import Counter
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
sys.path.insert(0,str(ROOT/"tools/dlls"))
from harness import Image, Machine, HEAP  # noqa: E402
from probe_original_bik_frame import (  # noqa: E402
    PIN_DLL,PIN_VIDEO,PACKET,SCRATCH_POINTERS,SCRATCH_FIRST,
    SCRATCH_SPACING,STATS,le32,
)
from unicorn import x86_const as X  # noqa: E402

Y_SHA="347b7e90cad7c4cc5d0e3e9b0541813b3a49605827aa72ec59c662ff1f24edaf"
MODE_RVA=0x25A32
END_RVA=0x25B17
FRAME_RVA=0x252C0
PIXELS=256
SAMPLES_PER_MODE=4


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("dll",type=Path)
    ap.add_argument("video",type=Path)
    ap.add_argument("--out",type=Path,default=Path("build/real-bink-block-modes.json"))
    args=ap.parse_args()
    if hashlib.sha256(args.dll.read_bytes()).hexdigest()!=PIN_DLL:
        raise SystemExit("Pinned Store Bink DLL does not match")
    video=args.video.read_bytes()
    if hashlib.sha256(video).hexdigest()!=PIN_VIDEO:
        raise SystemExit("Pinned original BO2 video does not match")
    start=le32(video,44)&~1
    end=le32(video,48)&~1
    packet=video[start:end]
    split=le32(packet,0)
    assert len(packet)==1676 and split==1372
    machine=Machine(Image(args.dll))
    image=machine.images[0]
    original_dest=bytes([0xB7])*(PIXELS*PIXELS)
    original_reference=bytes([0x80])*(PIXELS*PIXELS)
    pending=[]
    samples={}
    mode_counts=Counter()
    addresses=[]
    failures=[]

    def tile(mu,addr):
        return b"".join(bytes(mu.mem_read(addr+PIXELS*y,8)) for y in range(8))

    def dispatch(mu):
        mode=mu.reg_read(X.UC_X86_REG_RAX)&0xFFFFFFFF
        if mode>9:
            failures.append(f"Invalid dispatch mode {mode}")
            return
        target=mu.reg_read(X.UC_X86_REG_R15)
        # The original reference-plane argument is the second 65536-byte
        # Machine.call byte buffer, aligned to the next 0x11000 heap page.
        reference=HEAP+0x11000+(target-HEAP)
        stride=mu.reg_read(X.UC_X86_REG_R13)&0xFFFFFFFF
        if stride!=PIXELS:
            failures.append(f"Unexpected stride {stride}")
            return
        offset=target-HEAP
        if offset<0 or offset+7*PIXELS+8>len(original_dest):
            failures.append(f"Destination offset outside 256x256: {offset}")
            return
        event={
            "mode":mode,
            "output_offset":offset,
            "x":offset%PIXELS,
            "y":offset//PIXELS,
            "before_hex":tile(mu,target).hex(),
            "reference_hex":tile(mu,reference).hex(),
        }
        pending.append(event)
        mode_counts[str(mode)]+=1

    def epilogue(mu):
        if not pending:
            failures.append("Tile epilogue without dispatch")
            return
        event=pending.pop()
        if len(samples.get(str(event["mode"]),[]))<SAMPLES_PER_MODE:
            target=HEAP+event["output_offset"]
            event["after_hex"]=tile(mu,target).hex()
            samples.setdefault(str(event["mode"]),[]).append(event)
        addresses.append((event["x"],event["y"],event["mode"]))

    hooks={
        image.base+MODE_RVA:dispatch,
        image.base+END_RVA:epilogue,
    }
    scratch=[SCRATCH_FIRST+i*SCRATCH_SPACING for i in range(9)]
    def setup(w):
        w.write(PACKET,packet)
        w.write(SCRATCH_POINTERS,struct.pack("<9Q",*scratch))
        w.u32(STATS,0)
    result,buffers,_,_=machine.call(image.base+FRAME_RVA,
        [original_dest,original_reference,PIXELS,PIXELS,PIXELS,
         PACKET+4,PACKET+split,0,STATS,0,1,SCRATCH_POINTERS,0,STATS],
        ret="uint64",setup=setup,at=hooks,limit=15_000_000)
    if isinstance(result,str):
        raise AssertionError(f"Original luma decoder failed: {result}")
    assert result==PACKET+split
    assert hashlib.sha256(buffers[0]).hexdigest()==Y_SHA
    if pending or failures:
        raise AssertionError(f"Unpaired tile events={len(pending)}, errors={failures[:5]}")
    assert sum(mode_counts.values())==len(addresses)
    assert sum(mode_counts.values())>0

    data={
        "source":"original Store Bink DLL executed on original BO2 media in Unicorn",
        "dll_sha256":PIN_DLL,
        "fixture":"la_pip_seq_6.bik",
        "fixture_sha256":PIN_VIDEO,
        "first_keyframe_packet_sha256":hashlib.sha256(packet).hexdigest(),
        "frame_decoder_rva":f"0x{FRAME_RVA:X}",
        "mode_dispatch_rva":f"0x{MODE_RVA:X}",
        "tile_epilogue_rva":f"0x{END_RVA:X}",
        "original_luma_sha256":Y_SHA,
        "block_modes":dict(sorted(mode_counts.items(),key=lambda kv:int(kv[0]))),
        "total_dispatches":sum(mode_counts.values()),
        "sample_blocks":samples,
        "tiles":addresses,
    }
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(data,indent=2)+"\n")
    print("Original BO2 first luma keyframe block mode counts:",
          json.dumps(data["block_modes"],sort_keys=True))
    print("Original frame Y SHA-256",Y_SHA,
          "total dispatched blocks",data["total_dispatches"])
    print("Real frame original mode/block samples:",args.out)


if __name__=="__main__":
    main()
