#!/usr/bin/env python3
"""Capture all 65 real Bink coefficient blocks from ORIGINAL BO2 keyframe.

Executes the original SHA256-pinned Store binkw64.dll FUN_1800252C0 on
la_pip_seq_6.bik first frame's luma payload using Unicorn. Hook the original
coefficient decoder FUN_18002FD80 at ENTRY and its three verified return
addresses in FUN_1800252C0. Save bitstream cache and word-pointer states
before and after each call, the original 128-byte coefficient buffer before
and after, and the exact real input compressed packet.

The result is a deterministic golden corpus for a future SOURCE translation
of FUN_18002FD80; this script itself runs the ORIGINAL DLL and does NOT
pretend the full original frame decoder was already reconstructed.
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
from harness import Image,Machine,OBJECTS  # noqa: E402
from probe_original_bik_frame import (  # noqa: E402
    PIN_DLL,PIN_VIDEO,PACKET,SCRATCH_POINTERS,SCRATCH_FIRST,
    SCRATCH_SPACING,STATS,le32,
)
from unicorn import x86_const as X  # noqa: E402

ORIGINAL_Y_SHA="347b7e90cad7c4cc5d0e3e9b0541813b3a49605827aa72ec59c662ff1f24edaf"
BLOCK_RVA=0x2FD80
FRAME_RVA=0x252C0
RETURN_SITES=(0x26469,0x26D44,0x26F40)

def metadata(buf,packet_base):
    cache,=struct.unpack_from("<I",buf,0)
    nxt,=struct.unpack_from("<Q",buf,8)
    avail,=struct.unpack_from("<I",buf,16)
    return {
        "cache_u32":cache,
        "next_word_packet_byte_offset":nxt-packet_base,
        "bits_available":avail,
        "original_bitreader_hex":buf.hex(),
    }

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("dll",type=Path)
    ap.add_argument("video",type=Path)
    ap.add_argument("--out",type=Path,default=Path("build/real-bink-coefficient-oracle.json"))
    args=ap.parse_args()
    if hashlib.sha256(args.dll.read_bytes()).hexdigest()!=PIN_DLL:
        raise SystemExit("Wrong original binkw64.dll")
    raw=args.video.read_bytes()
    if hashlib.sha256(raw).hexdigest()!=PIN_VIDEO:
        raise SystemExit("Wrong original la_pip_seq_6.bik")
    image=Image(args.dll)
    machine=Machine(image)
    frame_start=le32(raw,44)&~1
    frame_end=le32(raw,48)&~1
    frame=raw[frame_start:frame_end]
    partition=le32(frame,0)
    assert len(frame)==1676 and partition==1372
    width=height=256
    init_y=bytes([0xB7])*(width*height)
    ref_y=bytes([0x80])*(width*height)
    pending=[]
    records=[]
    return_addresses={image.base+r for r in RETURN_SITES}

    def entered(mu):
        dest=mu.reg_read(X.UC_X86_REG_RCX)
        bitsptr=mu.reg_read(X.UC_X86_REG_RDX)
        sp=mu.reg_read(X.UC_X86_REG_RSP)
        retaddr=struct.unpack("<Q",bytes(mu.mem_read(sp,8)))[0]
        if retaddr not in return_addresses:
            raise AssertionError(f"Unexpected original 0x2FD80 return address {retaddr:#x}")
        pending.append({
            "dest":dest,"bitsptr":bitsptr,
            "retaddr":retaddr,
            "before_output":bytes(mu.mem_read(dest,128)),
            "before_bits":bytes(mu.mem_read(bitsptr,24)),
        })

    def returning(mu):
        if not pending:
            raise AssertionError("Coefficient return without captured entry")
        pre=pending.pop()
        ip=mu.reg_read(X.UC_X86_REG_RIP)
        if ip!=pre["retaddr"]:
            raise AssertionError(f"Coefficient return site mismatch {ip:#x}")
        after_out=bytes(mu.mem_read(pre["dest"],128))
        after_bits=bytes(mu.mem_read(pre["bitsptr"],24))
        records.append({
            "index":len(records),
            "return_rva":f"0x{ip-image.base:X}",
            "input_bitreader":metadata(pre["before_bits"],PACKET),
            "output_bitreader":metadata(after_bits,PACKET),
            "coeff_buffer_before_hex":pre["before_output"].hex(),
            "coeff_buffer_after_hex":after_out.hex(),
            "changed_output_bytes":sum(a!=b for a,b in zip(
                pre["before_output"],after_out)),
        })

    hooks={image.base+BLOCK_RVA:entered}
    hooks.update({returnsite:returning for returnsite in return_addresses})
    scratch=[SCRATCH_FIRST+i*SCRATCH_SPACING for i in range(9)]
    def setup(writer):
        writer.write(PACKET,frame)
        writer.write(SCRATCH_POINTERS,struct.pack("<9Q",*scratch))
        writer.u32(STATS,0)
    result,buffers,_,_=machine.call(
        image.base+FRAME_RVA,
        [init_y,ref_y,width,height,width,
         PACKET+4,PACKET+partition,0,STATS,0,1,
         SCRATCH_POINTERS,0,STATS],
        "uint64",setup=setup,at=hooks,limit=15_000_000)
    if isinstance(result,str):
        raise AssertionError(f"Original video decoder failed: {result}")
    if result!=PACKET+partition or pending:
        raise AssertionError("Original decoder did not finish first luma partition")
    y_sha=hashlib.sha256(buffers[0]).hexdigest()
    if y_sha!=ORIGINAL_Y_SHA:
        raise AssertionError(f"Original decoded Y plane drifted: {y_sha}")
    if len(records)!=65:
        raise AssertionError(f"Expected 65 real coefficient block calls, observed {len(records)}")
    for item in records:
        for field in ("input_bitreader","output_bitreader"):
            p=item[field]["next_word_packet_byte_offset"]
            if p<4 or p>len(frame)+16:
                raise AssertionError(f"Coefficient bitstream escapes packet {item['index']}")

    artifact={
        "source":"pinned ORIGINAL Store Bink DLL executed under Unicorn, not reconstructed source",
        "dll_sha256":PIN_DLL,
        "fixture":"la_pip_seq_6.bik",
        "fixture_sha256":PIN_VIDEO,
        "frame":0,
        "dimensions":[width,height],
        "frame_packet_offset":frame_start,
        "frame_packet_bytes_hex":frame.hex(),
        "luma_partition_bytes":partition,
        "original_luma_sha256":y_sha,
        "original_decoder_rva":"0x252C0",
        "original_coefficient_function_rva":"0x2FD80",
        "coefficient_calls":len(records),
        "calls":records,
    }
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(artifact,indent=2)+"\n")
    print(f"Captured {len(records)} real original Bink 0x2FD80 coefficient "
          f"block before/after states from frame 0; Y SHA256={y_sha}")
    print("Return sites:", sorted(set(r["return_rva"] for r in records)))
    print("Golden coefficient oracle:",args.out)

if __name__=="__main__":main()
