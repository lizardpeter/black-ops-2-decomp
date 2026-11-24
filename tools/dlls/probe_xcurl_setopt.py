#!/usr/bin/env python3
"""Read-only instruction evidence for the pinned XCurl option path and Store callers.
No original code is executed. Decode .pdata function ranges, not guessed lengths.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP, X86_REG_EDX, X86_REG_RDX

ROOT=Path(__file__).resolve().parents[2]
SHA="65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551"
def load(path):
    pe=pefile.PE(str(path))
    assert pe.FILE_HEADER.Machine==0x8664 and pe.OPTIONAL_HEADER.Magic==0x20b
    return pe
def decoder():
    md=Cs(CS_ARCH_X86,CS_MODE_64)
    md.detail=True
    return md
def functions(pe):
    return {e.struct.BeginAddress:e.struct.EndAddress for e in pe.DIRECTORY_ENTRY_EXCEPTION}
def imported(pe,dll):
    return {i.address:i.name.decode() for d in pe.DIRECTORY_ENTRY_IMPORT
            if d.dll.decode().lower()==dll for i in d.imports if i.name}
def instructions(pe,start,end):
    return list(decoder().disasm(pe.get_data(start,end-start),pe.OPTIONAL_HEADER.ImageBase+start))
def format_ins(pe,i):
    return f"{i.address-pe.OPTIONAL_HEADER.ImageBase:08x} {i.mnemonic:10s} {i.op_str}"
def function_evidence(pe,start,ranges):
    assert start in ranges, ("missing unwind entry",hex(start))
    ins=instructions(pe,start,ranges[start])
    assert ins
    base=pe.OPTIONAL_HEADER.ImageBase
    targets=sorted({i.operands[0].imm-base for i in ins
                    if i.mnemonic=="call" and i.operands[0].type==CS_OP_IMM})
    return {"rva":hex(start),"end":hex(ranges[start]),"direct_calls":[hex(t) for t in targets],
            "instructions":[format_ins(pe,i) for i in ins]},targets
def call_evidence(pe,rva,expected,iat):
    base=pe.OPTIONAL_HEADER.ImageBase
    # Decode from the containing unwind function, avoiding ambiguous backward
    # instruction boundary guesses.
    ranges=functions(pe)
    matches=[(s,e) for s,e in ranges.items() if s<=rva<e]
    assert len(matches)==1,(hex(rva),matches)
    s,e=matches[0]
    ins=instructions(pe,s,e)
    k=next((j for j,i in enumerate(ins) if i.address==base+rva),None)
    assert k is not None,(hex(rva),"not on decoded instruction boundary")
    call=ins[k]
    assert call.mnemonic=="call",(hex(rva),call.mnemonic)
    op=call.operands[0]
    target=None
    if op.type==CS_OP_MEM and op.mem.base==X86_REG_RIP:
        target=iat.get(call.address+call.size+op.mem.disp)
    elif op.type==CS_OP_IMM:
        # Some clients use a direct call to a single RIP-relative IAT jump.
        thunk=next(iter(instructions(pe,op.imm-base,op.imm-base+16)),None)
        if thunk and thunk.mnemonic=="jmp" and thunk.operands[0].type==CS_OP_MEM and thunk.operands[0].mem.base==X86_REG_RIP:
            target=iat.get(thunk.address+thunk.size+thunk.operands[0].mem.disp)
    assert target==expected,(hex(rva),target,expected)
    prior=ins[max(0,k-24):k]
    # The option is the second x64 integer argument. Only accept a literal
    # assignment to EDX/RDX after the last call/control-flow instruction.
    option=None
    for i in reversed(prior):
        if i.mnemonic.startswith(("call","j","ret")):break
        _,writes=i.regs_access()
        if X86_REG_EDX in writes or X86_REG_RDX in writes:
            if i.mnemonic=="mov" and len(i.operands)==2 and i.operands[1].type==CS_OP_IMM:
                option=i.operands[1].imm
            elif i.mnemonic=="xor" and len(i.operands)==2 and all(o.type==CS_OP_REG for o in i.operands) and i.operands[0].reg==i.operands[1].reg:
                option=0
            break
    return {"rva":hex(rva),"option":option,"instructions":[format_ins(pe,i) for i in prior+[call]]}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--dll",type=Path,required=True)
    p.add_argument("--originals",type=Path,required=True)
    p.add_argument("--out",type=Path,required=True)
    a=p.parse_args()
    assert hashlib.sha256(a.dll.read_bytes()).hexdigest()==SHA
    pe=load(a.dll)
    ranges=functions(pe)
    exports={e.name.decode():e.address for e in pe.DIRECTORY_ENTRY_EXPORT.symbols if e.name}
    roots=[exports["curl_easy_setopt"],0xcbf8,0xeaf8]
    evidence={}
    for r in roots:
        item,targets=function_evidence(pe,r,ranges)
        evidence[hex(r)]=item
        for t in targets:
            if t in ranges and hex(t) not in evidence:
                evidence[hex(t)]=function_evidence(pe,t,ranges)[0]
    callers=json.loads((ROOT/"docs/xcurl-store-callers.json").read_text())
    observed={}
    for b in callers["builds"]:
        name=b["build"]
        f=a.originals/(name+".exe")
        client=load(f)
        iat=imported(client,"xcurl.dll")
        assert set(iat.values())==set(callers["imports"]),(name,iat)
        observed[name]={"sha256":hashlib.sha256(f.read_bytes()).hexdigest()}
        for kind in ("setopt","getinfo"):
            records=[]
            for site in b[kind]:
                rec=call_evidence(client,int(site["call_rva"],16),"curl_easy_"+kind,iat)
                if "option" in site and rec["option"] is not None:
                    assert rec["option"]==site["option"],(name,site,rec)
                records.append(rec)
            observed[name][kind]=records
    result={"dll_sha256":SHA,"exports":{"curl_easy_setopt":hex(roots[0])},
            "functions":evidence,"clients":observed}
    a.out.parent.mkdir(parents=True,exist_ok=True)
    a.out.write_text(json.dumps(result,indent=2)+"\n")
    for r in roots:
        print("\nFUNCTION",hex(r))
        print("\n".join(evidence[hex(r)]["instructions"]))
    for name in ("curl_easy_init","curl_easy_cleanup","curl_multi_init","curl_multi_cleanup",
                 "curl_multi_add_handle","curl_multi_remove_handle","curl_multi_perform",
                 "curl_multi_poll","curl_multi_info_read"):
        r=exports[name]
        if r in ranges:
            item,targets=function_evidence(pe,r,ranges)
            print("\nLIFECYCLE",name,hex(r))
            print("\n".join(item["instructions"]))
            result["functions"][hex(r)]=item
            result["exports"][name]=hex(r)
            for t in targets:
                if t in ranges and hex(t) not in result["functions"]:
                    result["functions"][hex(t)]=function_evidence(pe,t,ranges)[0]
    a.out.write_text(json.dumps(result,indent=2)+"\n")
    for n,b in observed.items():
        print(n, {k:[x["option"] for x in b[k]] for k in ("setopt","getinfo")})
    # Unresolved values remain explicit rather than being copied from MP.
    mp=observed["t6mp_msstore"]
    for k in ("setopt","getinfo"):
        if any(x["option"] is None for x in mp[k]):
            print("Incomplete literal recovery for MP",k)
        for n,b in observed.items():
            if n=="t6mp_msstore":continue
            pairs=list(zip(mp[k],b[k]))
            assert all(x["option"]==y["option"] for x,y in pairs if x["option"] is not None and y["option"] is not None),(n,k)
    print("Read-only option-path evidence saved; unresolved values are null.")
if __name__=="__main__":main()
