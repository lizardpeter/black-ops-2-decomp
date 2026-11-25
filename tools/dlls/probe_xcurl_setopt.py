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
# Constant propagation through decoded control flow. Missing register values
# represent unknown; join keeps only constants equal on all incoming paths.
# Calls clobber Win64 volatile GPRs, and unsupported writes invalidate the
# entire register (including partial writes). No external code is executed.
def canonical(name):
    families=(("rax","eax","ax","al","ah"),("rbx","ebx","bx","bl","bh"),
              ("rcx","ecx","cx","cl","ch"),("rdx","edx","dx","dl","dh"),
              ("rsi","esi","si","sil"),("rdi","edi","di","dil"),
              ("rbp","ebp","bp","bpl"),("rsp","esp","sp","spl"))
    for family in families:
        if name in family:return family[0]
    for i in range(8,16):
        if name in (f"r{i}",f"r{i}d",f"r{i}w",f"r{i}b"):return f"r{i}"
    return None

VOLATILE={"rax","rcx","rdx","r8","r9","r10","r11"}
def constant_args(ins,k):
    from collections import deque
    from itertools import product
    LIMIT=8
    by_addr={i.address:j for j,i in enumerate(ins)}
    branch_targets={i.operands[0].imm for i in ins
                    if i.mnemonic.startswith(("j","loop")) and i.operands and i.operands[0].type==CS_OP_IMM}
    # Unknown indirect branches could enter any block with unknown state.
    if any(i.mnemonic.startswith(("j","loop")) and
           (not i.operands or i.operands[0].type!=CS_OP_IMM) for i in ins):return None
    incoming={0:{}};pending=deque([0])
    def finite(values):
        result=frozenset(values)
        return result if len(result)<=LIMIT else None
    def merge(j,values):
        if j is None or not 0<=j<len(ins):return
        if j not in incoming:
            incoming[j]=values.copy();pending.append(j);return
        old=incoming[j];new={}
        for r,v in old.items():
            if r in values:
                merged=finite(v|values[r])
                if merged is not None:new[r]=merged
        if new!=old:incoming[j]=new;pending.append(j)
    def value(i,operand,state):
        if operand.type==CS_OP_IMM:return frozenset([operand.imm])
        if operand.type==CS_OP_REG:
            v=state.get(canonical(i.reg_name(operand.reg)))
            if v is not None and operand.size in (4,8):
                return finite(n&((1<<(8*operand.size))-1) for n in v)
        return None
    def refine_equality(j,values,equal):
        # Only use a directly preceding 64-bit compare when the branch is
        # not itself a target of any other decoded branch. A 32-bit compare
        # would not prove the upper half of the register.
        if not j or ins[j].address in branch_targets:return values
        cmp=ins[j-1];ops=cmp.operands
        if cmp.mnemonic!="cmp" or len(ops)!=2 or ops[0].type!=CS_OP_REG or ops[0].size!=8 or ops[1].type!=CS_OP_IMM:return values
        r=canonical(cmp.reg_name(ops[0].reg))
        if not r:return values
        v=ops[1].imm&((1<<64)-1);old=values.get(r)
        if equal:
            if old is not None and v not in old:return None
            return dict(values,**{r:frozenset([v])})
        if old is None:return values
        remain=old-{v}
        return dict(values,**{r:remain}) if remain else None
    while pending:
        j=pending.popleft();i=ins[j];state=incoming[j];out=state.copy()
        op=i.operands;mn=i.mnemonic
        _,writes=i.regs_access()
        for reg in writes:
            name=canonical(i.reg_name(reg))
            if name:out.pop(name,None)
        if mn.startswith("call"):
            for reg in VOLATILE:out.pop(reg,None)
        if len(op)>=1 and op[0].type==CS_OP_REG and op[0].size in (4,8):
            dst=canonical(i.reg_name(op[0].reg));v=None
            if dst:
                if mn in ("mov","movabs") and len(op)==2:v=value(i,op[1],state)
                elif mn=="xor" and len(op)==2 and op[1].type==CS_OP_REG and op[0].reg==op[1].reg:v=frozenset([0])
                elif mn=="lea" and len(op)==2 and op[1].type==CS_OP_MEM:
                    m=op[1].mem
                    base=state.get(canonical(i.reg_name(m.base))) if m.base else frozenset([0])
                    index=state.get(canonical(i.reg_name(m.index))) if m.index else frozenset([0])
                    if base is not None and index is not None and not m.segment:v=finite(a+b*m.scale+m.disp for a,b in product(base,index))
                elif mn in ("add","sub","and","or","xor") and len(op)==2:
                    a=value(i,op[0],state);b=value(i,op[1],state)
                    if a is not None and b is not None:
                        func={"add":lambda x,y:x+y,"sub":lambda x,y:x-y,"and":lambda x,y:x&y,"or":lambda x,y:x|y,"xor":lambda x,y:x^y}[mn]
                        v=finite(func(x,y) for x,y in product(a,b))
                if v is not None:out[dst]=finite(n&((1<<(8*op[0].size))-1) for n in v)
        if mn.startswith(("ret","iret")) or mn in ("int3","ud2"):continue
        if mn.startswith("j") or mn.startswith("loop"):
            yes=out;no=out
            if mn in ("je","jz","jne","jnz"):
                is_equal=mn in ("je","jz")
                yes=refine_equality(j,out,is_equal);no=refine_equality(j,out,not is_equal)
            if yes is not None:merge(by_addr.get(op[0].imm),yes)
            if mn in ("jmp","ljmp") or no is None:continue
            merge(j+1,no)
        else:merge(j+1,out)
    v=incoming.get(k,{}).get("rdx")
    return None if v is None else sorted({n&0xffffffff for n in v})

def constant_arg(ins,k):
    values=constant_args(ins,k)
    return values[0] if values is not None and len(values)==1 else None

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
    options=constant_args(ins,k)
    option=options[0] if options is not None and len(options)==1 else None
    return {"rva":hex(rva),"option":option,"options":options,"proof":"control-flow value set" if options is not None else None,
            "instructions":[format_ins(pe,i) for i in prior+[call]]}

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
                if "option" in site and rec["options"] is not None:
                    assert site["option"] in rec["options"],(name,site,rec)
                if "options" in site and rec["options"] is not None:
                    assert rec["options"]==site["options"],(name,site,rec)
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
        print(n, {k:[x["options"] for x in b[k]] for k in ("setopt","getinfo")})
    # Unresolved values remain explicit rather than being copied from MP.
    mp=observed["t6mp_msstore"]
    for k in ("setopt","getinfo"):
        if any(x["options"] is None for x in mp[k]):
            print("Incomplete literal recovery for MP",k)
        for n,b in observed.items():
            if n=="t6mp_msstore":continue
            pairs=list(zip(mp[k],b[k]))
            assert all(x["options"]==y["options"] for x,y in pairs if x["options"] is not None and y["options"] is not None),(n,k)
    print("Read-only option-path evidence saved; unresolved values are null.")
if __name__=="__main__":main()
