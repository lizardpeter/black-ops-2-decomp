#!/usr/bin/env python3
"""Trace original Bink frame's 9 real entropy refill stages per tile row.

Feeds the first real BO2 la_pip_seq_6.bik Y partition through pinned
ORIGINAL FUN_1800252C0 under Unicorn. Hooks nine native return
addresses immediately after each entropy decoder call. At each return,
reads original 24-byte LSB-first bitreader at original stack [RSP+0x48]
and records compressed next-word offset, available-bit count and cache.

The original full-frame output must match its pinned Y SHA256.
This oracle is intentionally separate from reconstructed C++; its output
pinpoints the FIRST entropy stage that diverges in native-frame port.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
sys.path.insert(0,str(ROOT/"tools/dlls"))
from harness import Image,Machine  # noqa: E402
from probe_original_bik_frame import (  # noqa: E402
    PIN_DLL,PIN_VIDEO,PACKET,SCRATCH_POINTERS,SCRATCH_FIRST,
    SCRATCH_SPACING,STATS,le32,
)
from unicorn import x86_const as X  # noqa: E402

Y_SHA="347b7e90cad7c4cc5d0e3e9b0541813b3a49605827aa72ec59c662ff1f24edaf"
AFTER_RETURNS={
    0x2588F:"mode_rle",0x258A4:"submode_rle",
    0x258C3:"adaptive_colors",0x258D8:"nibble_patterns",
    0x258ED:"signed_motion_x",0x25902:"signed_motion_y",
    0x25947:"delta_16_dc",0x2595C:"delta_16_other",
    0x25971:"rle_coefficients",
}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("dll",type=Path)
    p.add_argument("video",type=Path)
    p.add_argument("--out",type=Path,
                   default=Path("build/original-bink-entropy-rows.json"))
    a=p.parse_args()
    if hashlib.sha256(a.dll.read_bytes()).hexdigest()!=PIN_DLL:
        raise SystemExit("Original Bink DLL mismatch")
    video=a.video.read_bytes()
    if hashlib.sha256(video).hexdigest()!=PIN_VIDEO:
        raise SystemExit("Original video mismatch")
    start,end=(le32(video,x)&~1 for x in (44,48))
    packet=video[start:end]
    split=le32(packet,0)
    assert len(packet)==1676 and split==1372
    img=Image(a.dll)
    machine=Machine(img)
    captured=[]
    stage_index=[0]
    def hook(rva,stage):
        def trace(mu):
            # FUN_1800252C0's standard frame-stack RSP+0x48 bit reader.
            sp=mu.reg_read(X.UC_X86_REG_RSP)
            b=bytes(mu.mem_read(sp+0x48,24))
            cache,=struct.unpack_from("<I",b,0)
            word,=struct.unpack_from("<Q",b,8)
            avail,=struct.unpack_from("<I",b,16)
            assert PACKET<=word<=PACKET+len(packet)+16
            assert 0<=avail<=63 and (word-PACKET)%4==0
            captured.append({
                "row":(stage_index[0]//9)*8,
                "stage":stage,"word":(word-PACKET)//4,
                "bits":avail,"cache":cache
            })
            stage_index[0]+=1
        return trace

    hooks={img.base+rva:hook(rva,stage) for rva,stage in AFTER_RETURNS.items()}
    scratch=[SCRATCH_FIRST+i*SCRATCH_SPACING for i in range(9)]
    def setup(w):
        w.write(PACKET,packet)
        w.write(SCRATCH_POINTERS,struct.pack("<9Q",*scratch))
        w.u32(STATS,0)
    result,bufs,_,_=machine.call(
        img.base+0x252C0,
        [bytes([0xB7])*65536,bytes([0x80])*65536,256,256,256,
         PACKET+4,PACKET+split,0,STATS,0,1,SCRATCH_POINTERS,0,STATS],
        ret="uint64",setup=setup,at=hooks,limit=15_000_000)
    assert result==PACKET+split
    assert hashlib.sha256(bufs[0]).hexdigest()==Y_SHA
    assert len(captured)==32*9 and stage_index[0]==32*9
    for r in captured:
        if r["row"]<=112:
            print(f"ORIG ENTROPY y={r['row']:3d} {r['stage']:<18} "
                  f"word={r['word']:3d} bits={r['bits']:2d} cache=0x{r['cache']:08x}")
    result={
        "origin":"pinned-original Bink DLL under Unicorn; not native source",
        "fixture_sha256":PIN_VIDEO,"dll_sha256":PIN_DLL,
        "y_oracle_sha256":Y_SHA,"row_stages":captured,
    }
    a.out.parent.mkdir(parents=True,exist_ok=True)
    a.out.write_text(json.dumps(result,indent=2)+"\n")
    print("ORIGINAL frame entropy trace PASS:",len(captured),"stage states")

if __name__=="__main__":main()
