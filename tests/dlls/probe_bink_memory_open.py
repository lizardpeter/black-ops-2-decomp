#!/usr/bin/env python3
"""Run the real Store BinkOpen memory-backed initializer under Unicorn.

The goal is recovering actual handle/bootstrap state, not validating a
replacement Open. Only allocation and clock boundaries are substituted.
Original code, including its frame-index and allocation-size calculation,
runs normally. Any unmodeled imported function is a verification failure.
No game data or vendor binary is emitted; only JSON layout evidence.
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
from harness import Image,Machine,HEAP
from differential_bink_resident_state import PIN
from fetch_bik_fixtures import FIXTURES
from unicorn import x86_const as X

ARENA=0x34000000
ARENA_SIZE=16*1024*1024
FLAGS=0x04000000 # actual in-memory branch at original 0x17A65

WORDS=(0,4,8,0x0c,0x10,0x14,0x18,0x1c,0x20,0x24,0x34,0x3c,
       0x110,0x114,0x120,0x124,0x128,0x12c,0x130,0x134,0x138,0x13c,
       0x140,0x144,0x148,0x1b8,0x1bc,0x1e8,0x1ec,0x270,0x27c,
       0x510,0x514,0x520,0x524,0x528,0x530,0x534,0x538,0x560,0x564,
       0x56c,0x570,0x574,0x578,0x57c,0x58c,0x59c)
POINTERS=(0xd0,0xd8,0xe0,0xe8,0xf0,0x100,0x108,0x198,0x1a0,0x1c0,
          0x1c8,0x1d0,0x1d8,0x200,0x220,0x228,0x4d8,0x4e0,0x4e8,
          0x4f0,0x4f8,0x500,0x508,0x5a0)

def run(dll,fixture,flags):
    data=fixture.read_bytes()
    pin=FIXTURES[fixture.name]
    assert hashlib.sha256(data).hexdigest()==pin
    assert hashlib.sha256(dll.read_bytes()).hexdigest()==PIN
    words=struct.unpack_from("<11I",data)
    assert words[0]==0x694b4942 and words[10]==0
    img=Image(dll)
    m=Machine(img,large_heap=True)
    m.mu.mem_map(ARENA,ARENA_SIZE)
    # Model the x64 Windows SYSTEM_INFO ABI, not a Bink internal routine.
    # Use a fixed AMD64 single-logical-CPU environment for reproducibility.
    def system_info(mu):
        address=mu.reg_read(X.UC_X86_REG_RCX)
        block=struct.pack('<HHIQQQIIIHH',9,0,4096,0x10000,0x7ffffffeffff,
                          1,1,8664,65536,6,0)
        assert len(block)==48
        mu.mem_write(address,block)
    modeled={'kernel32.dll!getsysteminfo'}
    m.on_import('kernel32.dll!GetSystemInfo',system_info)
    cursor=ARENA
    allocations=[]
    ticks=[]
    def alloc(mu):
        nonlocal cursor
        size=mu.reg_read(X.UC_X86_REG_ECX)
        assert size and size<ARENA_SIZE
        address=(cursor+63)&~63
        cursor=address+size+64
        assert cursor<=ARENA+ARENA_SIZE
        # Original initializer owns all initialization; allocator boundary
        # supplies zero-filled committed pages, as the default heap path does.
        mu.mem_write(address,bytes(size))
        allocations.append({"offset":address-ARENA,"requested_bytes":size})
        return address
    def clock(mu):
        tick=100000+len(ticks)*3
        ticks.append(tick)
        return tick
    result={"fixture":fixture.name,"flags":flags,"header":list(words),
            "allocations":allocations,"timer_calls":ticks}
    ret,bufs,_,_=m.call(img.exports["BinkOpen"],[data,flags],"uint64",
                       fakes={img.base+0x1010:alloc,img.base+0x1FF00:clock},
                       limit=60_000_000)
    result["original_return"]=ret
    result["imports_called"]=m.imports_called
    result["unmodeled_imports"]=[name for name in m.imports_called if name.lower() not in modeled]
    if not isinstance(ret,int) or not ARENA<=ret<cursor:
        result["verified"]=False
        return result
    raw=bytes(m.mu.mem_read(ret,0x5B8))
    def u32(off):return struct.unpack_from("<I",raw,off)[0]
    def p64(off):return struct.unpack_from("<Q",raw,off)[0]
    # Normalize pointers to independently allocated state or input file.
    # Machine's large heap starts at 0x32000000, not default HEAP.
    input_base=m.heap
    def normalize(p):
        if not p:return None
        if ARENA<=p<cursor:return {"allocation_offset":p-ARENA}
        if input_base<=p<input_base+len(data):return {"file_offset":p-input_base}
        if img.base<=p<img.base+img.size:return {"original_rva":p-img.base}
        return {"other":hex(p)}
    result["handle_offset"]=ret-ARENA
    result["fields"]={hex(off):u32(off) for off in WORDS}
    result["pointers"]={hex(off):normalize(p64(off)) for off in POINTERS}
    n=words[2]
    actual_index=p64(0x1a0)
    index=bytes(m.mu.mem_read(actual_index,(n+1)*4))
    expected_index=data[44:44+(n+1)*4]
    result["index_matches"]=index==expected_index
    result["input_unchanged"]=bool(bufs) and bufs[0]==data
    result["frame_metadata_matches"]=(u32(0)==words[5] and u32(4)==words[6]
        and u32(8)==n and u32(0x0c)==1 and u32(0x14)==words[7] and u32(0x18)==words[8])
    # Do not bless an emulated path which depended on an unmodeled OS call.
    result["verified"]=result["index_matches"] and result["input_unchanged"] and result["frame_metadata_matches"] and not result["unmodeled_imports"]
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("dll",type=Path)
    p.add_argument("fixture",type=Path)
    p.add_argument("--out",type=Path,required=True)
    p.add_argument("--flags",type=lambda s:int(s,0),default=FLAGS)
    a=p.parse_args()
    result=run(a.dll,a.fixture,a.flags)
    a.out.parent.mkdir(parents=True,exist_ok=True)
    a.out.write_text(json.dumps(result,indent=2)+"\n")
    print(json.dumps(result,indent=2),flush=True)
    if not result["verified"]:raise SystemExit("Original memory-open probe not yet verified")
    print("Original memory-backed BinkOpen PASS")
if __name__=="__main__":main()
