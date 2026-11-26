#!/usr/bin/env python3
"""Verify native renderer aliases and cross-compile the MP body with the real linker rules."""
import csv
import json
import pathlib
import subprocess
import sys
import tempfile
import pefile

from verify_store_renderer_prefix import ROOT, GLOBALS as EARLY
from verify_store_renderer_workers import GLOBALS as WORKERS
from verify_store_renderer_post_worker import GLOBALS as LATE
from verify_store_renderer_complete import VIS, TAIL
sys.path.insert(0,str(ROOT/"tools"))
import symbols
import build_recon

def main():
    exe=pathlib.Path(sys.argv[1])
    evidence=json.loads(pathlib.Path(sys.argv[2]).read_text())
    pe=pefile.PE(str(exe),fast_load=True)
    base=pe.OPTIONAL_HEADER.ImageBase
    rows=list(csv.DictReader((ROOT/"records/store-renderer-bindings.tsv").open(),delimiter="\t"))
    refs={int(x["va"],16) for x in evidence["references"]}
    calls={int(x["target"],16) for x in evidence["calls"] if x["target"]}
    slots=set()
    data=set()
    for r in rows:
        va=int(r["va"],16)
        role=r["role"]
        name=r["symbol"]
        assert symbols.resolve("t6mp_msstore",name)==(va,r["kind"]),name
        assert symbols.resolve("t6zm_msstore",name) is None,name
        sec=pe.get_section_by_rva(va-base)
        assert sec is not None,name
        if r["kind"]=="function":
            assert sec.Characteristics&0x20000000,name
            assert va==(int(evidence["start"],16) if role=="entry" else va) and (role=="entry" or va in calls),name
        else:
            # The worker loop uses image-base + index + displacement,
            # not RIP-relative operands. Independently check those exact
            # instructions rather than pretending the RIP inventory covers them.
            if name in ("Store_Renderer_WorkerNames","store_r_smp_worker_thread"):
                offset,expected={
                    "Store_Renderer_WorkerNames":(0x70fa93,"4b8b8c2ff0d39a00"),
                    "store_r_smp_worker_thread":(0x70fabd,"4b89842fb06e7e02"),
                }[name]
                assert pe.get_data(offset,len(bytes.fromhex(expected)))==bytes.fromhex(expected),name
                assert int.from_bytes(bytes.fromhex(expected)[4:],"little")==va-base,name
            else:
                assert va in refs,name
            data.add(name)
            if role=="result":
                assert sec.Characteristics&0x80000000,name
                slots.update(va-base+8*i for i in range(int(r["count"])))
    assert slots==set(EARLY+WORKERS+LATE+VIS+TAIL)
    assert len(slots)==256 and len(rows)==282
    with tempfile.TemporaryDirectory() as tmp:
        tmp=pathlib.Path(tmp)
        flags=["-DT6_STORE","-DT6_STORE_RENDERER_NATIVE","-ffunction-sections"]
        body=tmp/"body.obj"
        hook=tmp/"hook.obj"
        build_recon.compile_unit(ROOT/"src/gfx_d3d/r_dvars_store.cpp",body,"x64",flags)
        build_recon.compile_unit(ROOT/"enhanced/gfx_d3d/r_dvars.cpp",hook,"x64",flags)
        asm=body.with_suffix(".s")
        count=build_recon.exe_relative_asm(asm,data)
        assert count>=256,count
        subprocess.run(["clang",build_recon.ARCH["x64"]["triple"],"-c",str(asm),"-o",str(body)],check=True)
        assert body.stat().st_size>0 and hook.stat().st_size>0
        print(f"{len(rows)} exact MP-only aliases; {len(slots)} result slots; {count} rebased native references; Windows x64 body and hook objects compiled")
if __name__=="__main__":
    main()
