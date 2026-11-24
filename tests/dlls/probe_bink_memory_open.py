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
    m.mu.mem_map(ARENA-0x1000,ARENA_SIZE+0x1000)
    # Model the x64 Windows SYSTEM_INFO ABI, not a Bink internal routine.
    # Use a fixed AMD64 single-logical-CPU environment for reproducibility.
    def system_info(mu):
        address=mu.reg_read(X.UC_X86_REG_RCX)
        block=struct.pack('<HHIQQQIIIHH',9,0,4096,0x10000,0x7ffffffeffff,
                          1,1,8664,65536,6,0)
        assert len(block)==48
        mu.mem_write(address,block)
    # Optional dynamic platform facilities are absent in the deterministic
    # environment. Record requested names, do not bypass any internal logic.
    dynamic_queries=[]
    def string(mu,p):
        if not p:return None
        if p<65536:return f"ordinal:{p}"
        return bytes(mu.mem_read(p,256)).split(b'\0',1)[0].decode('ascii',errors='replace')
    def module(mu):
        name=string(mu,mu.reg_read(X.UC_X86_REG_RCX))
        dynamic_queries.append(["module",name])
        # Kernel32 is loaded; other optional DLLs may legitimately be absent.
        mu.reg_write(X.UC_X86_REG_RAX,0x70000000 if name is None or name.lower() in ("kernel32","kernel32.dll") else 0)
    def proc(mu):
        name=string(mu,mu.reg_read(X.UC_X86_REG_RDX))
        dynamic_queries.append(["proc",name])
        mu.reg_write(X.UC_X86_REG_RAX,0)
    modeled={'kernel32.dll!getsysteminfo','kernel32.dll!getmodulehandlea','kernel32.dll!getprocaddress'}
    m.on_import('kernel32.dll!GetSystemInfo',system_info)
    m.on_import('kernel32.dll!GetModuleHandleA',module)
    m.on_import('kernel32.dll!GetProcAddress',proc)
    # Single-threaded Win32 synchronization model. Track ownership/counts
    # and reject waits on objects which cannot actually be acquired.
    objects={}
    sync_events=[]
    def new_object(kind,value,maximum):
        handle=0x71000000+len(objects)*16
        objects[handle]=[kind,value,maximum]
        sync_events.append(["create",kind,value,maximum])
        m.mu.reg_write(X.UC_X86_REG_RAX,handle)
    def mutex(mu):
        new_object("mutex",int(bool(mu.reg_read(X.UC_X86_REG_EDX))),1)
    def semaphore(mu):
        value=mu.reg_read(X.UC_X86_REG_EDX);maximum=mu.reg_read(X.UC_X86_REG_R8D)
        assert 0<=value<=maximum
        new_object("semaphore",value,maximum)
    def wait(mu):
        handle=mu.reg_read(X.UC_X86_REG_RCX)
        timeout=mu.reg_read(X.UC_X86_REG_EDX)
        kind,value,maximum=objects[handle]
        if kind=="mutex":objects[handle][1]+=1;ret=0
        elif value:objects[handle][1]-=1;ret=0
        else:ret=258
        sync_events.append(["wait",kind,timeout,ret])
        mu.reg_write(X.UC_X86_REG_RAX,ret)
    def release_mutex(mu):
        handle=mu.reg_read(X.UC_X86_REG_RCX)
        kind,value,maximum=objects[handle]
        assert kind=="mutex" and value
        objects[handle][1]-=1
        sync_events.append(["release",kind])
        mu.reg_write(X.UC_X86_REG_RAX,1)
    def release_semaphore(mu):
        handle=mu.reg_read(X.UC_X86_REG_RCX)
        count=mu.reg_read(X.UC_X86_REG_EDX)
        prev=mu.reg_read(X.UC_X86_REG_R8)
        kind,value,maximum=objects[handle]
        assert kind=="semaphore" and value+count<=maximum
        if prev:mu.mem_write(prev,struct.pack("<I",value))
        objects[handle][1]+=count
        sync_events.append(["release",kind,count])
        mu.reg_write(X.UC_X86_REG_RAX,1)
    def close_handle(mu):
        handle=mu.reg_read(X.UC_X86_REG_RCX)
        assert handle in objects
        sync_events.append(["close",objects[handle][0]])
        objects[handle][0]="closed"
        mu.reg_write(X.UC_X86_REG_RAX,1)
    for name,callback in (("CreateMutexA",mutex),("CreateSemaphoreA",semaphore),
            ("WaitForSingleObject",wait),("ReleaseMutex",release_mutex),
            ("ReleaseSemaphore",release_semaphore),("CloseHandle",close_handle)):
        m.on_import("kernel32.dll!"+name,callback)
        modeled.add("kernel32.dll!"+name.lower())
    cursor=ARENA
    allocations=[]
    freed=[]
    free_target=img.base+0x17950 # distinct instrumented allocator callback address
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
        # Original allocation footer: type 3 (custom allocator), raw-pointer
        # delta byte, deallocation callback at user_ptr-12.
        mu.mem_write(address-12,struct.pack('<Q',free_target))
        mu.mem_write(address-2,b'\x03\x20')
        allocations.append({"offset":address-ARENA,"requested_bytes":size})
        return address
    def clock(mu):
        tick=100000+len(ticks)*3
        ticks.append(tick)
        return tick
    result={"fixture":fixture.name,"flags":flags,"header":list(words),
            "allocations":allocations,"timer_calls":ticks,"dynamic_queries":dynamic_queries,"sync_events":sync_events}
    ret,bufs,global_writes,_=m.call(img.exports["BinkOpen"],[data,flags],"uint64",
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
    descriptor=p64(0xd0)
    if descriptor:
        desc=bytes(m.mu.mem_read(descriptor,0xd8))
        result["descriptor"]={"header":list(struct.unpack_from("<6I",desc)),
            "slots":[[{"allocate":struct.unpack_from("<I",desc,0x18+s*0x60+p*0x18)[0],
                       "data":normalize(struct.unpack_from("<Q",desc,0x20+s*0x60+p*0x18)[0]),
                       "pitch":struct.unpack_from("<I",desc,0x28+s*0x60+p*0x18)[0]}
                      for p in range(4)] for s in range(2)]}
    # Replay original image globals, retaining actual original allocations.
    # Machine.call resets its input heap, so restore the unchanged movie bytes.
    def setup_close(w):
        m.mu.mem_write(input_base,data)
        for address,value in global_writes.items():w.write(address,value)
    def free(mu):
        raw=mu.reg_read(X.UC_X86_REG_RCX)
        user=raw+32
        assert user in [ARENA+a["offset"] for a in allocations],hex(raw)
        assert user not in freed,hex(raw)
        freed.append(user)
        return 0
    closed,_,_,_=m.call(img.exports["BinkClose"],[ret],"void",setup=setup_close,
        fakes={img.base+0x1FF00:clock,free_target:free},limit=60_000_000)
    result["close_return"]=closed
    result["close_imports_called"]=m.imports_called
    result["freed_offsets"]=[a-ARENA for a in freed]
    result["close_verified"]=(closed is None and sorted(result["freed_offsets"])==
        sorted(a["offset"] for a in allocations) and
        all(name.lower() in modeled for name in m.imports_called))
    result["verified"]=result["verified"] and result["close_verified"]
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
