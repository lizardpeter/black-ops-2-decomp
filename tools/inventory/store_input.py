#!/usr/bin/env python3
"""Recover exact Store MP input instruction bodies and call boundaries.

The PE is hash-pinned. Function starts and instruction boundaries come from
the existing Ghidra inventory, not guessed function sizes. A missing body
is a hard error; named callees are annotations, never substitute evidence.
"""
import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path
import capstone as cs
from capstone import x86_const as x
import pefile
from store_dvar_next import ROOT, DIGEST, HEADER, INSN

TARGETS={
    0x140192650:"CG_IsSprinting",0x1401926D0:"CL_KeyState",
    0x140192B70:"CL_UpdateAirburstMarkDistance",0x140192CD0:"CL_AddCurrentStanceToCmd",
    0x140192D50:"CL_KeyMove",0x1401930F0:"CL_MouseMove",
    0x1401939D0:"CL_GamepadMove",0x140193FE0:"CL_CmdButtons",
    0x140194460:"CG_HandleLocationSelectionInput",0x140194A70:"CL_HandleSpectatorInput",
    0x140194E20:"CL_CreateCmd",0x1401954D0:"CL_CreateNewCommands",
    0x140195690:"CL_ShutdownInput",0x1401974A0:"CL_Input",
    0x140197520:"CL_ExecBinding",0x140198EE0:"CL_SpectatorInput",
    0x1401A3350:"CL_FinishMove",0x1400F1E90:"CG_ShouldUpdateViewAngles",
    0x140499A10:"IN_MouseMove",0x14049A0A0:"IN_Frame",
    0x1403946A0:"Com_LocalClient_LastInput_Set",0x1406AC5D0:"Store_LUIMouseEvent",
}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("exe",type=Path);p.add_argument("out",type=Path);a=p.parse_args()
    raw=a.exe.read_bytes()
    assert hashlib.sha256(raw).hexdigest()==DIGEST,"wrong Store MP original"
    pe=pefile.PE(data=raw,fast_load=True);base=pe.OPTIONAL_HEADER.ImageBase
    assert base==0x140000000
    md=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_64);md.detail=True
    def read(va,n):return pe.get_data(va-base,n)
    names={}
    for line in (ROOT/"records/names.tsv").read_text().splitlines()[1:]:
        cells=line.split("\t")
        if len(cells)>4 and cells[0]=="t6mp_msstore":
            names[int(cells[1],16)]=cells[3]
    names.update(TARGETS)
    bodies=defaultdict(list)
    shards=sorted((ROOT/"analysis/t6mp_msstore/disasm/_unattributed").glob("*.asm"))
    assert len(shards)==9
    for shard in shards:
        owner=None
        with shard.open(encoding="utf-8") as f:
            for line in f:
                h=HEADER.match(line)
                if h:owner=int(h[1],16);continue
                if owner not in TARGETS:continue
                m=INSN.match(line)
                if not m:continue
                va=int(m[1],16)
                ins=next(md.disasm(read(va,15),va,count=1),None)
                assert ins is not None,(hex(owner),hex(va))
                row={"va":hex(va),"bytes":ins.bytes.hex(),"op":ins.mnemonic,"args":ins.op_str,"rip":[]}
                for op in ins.operands:
                    if op.type==x.X86_OP_MEM and op.mem.base==x.X86_REG_RIP:
                        target=va+ins.size+op.mem.disp
                        ref={"va":hex(target),"size":op.size,"access":op.access}
                        if target in names:ref["name"]=names[target]
                        sec=pe.get_section_by_rva(target-base)
                        if sec is not None and not sec.Characteristics&0x80000000:
                            ref["bytes"]=read(target,op.size).hex()
                            if ins.mnemonic=="lea":
                                buf=read(target,256).split(b"\0",1)[0]
                                if buf and all(32<=b<127 for b in buf):ref["string"]=buf.decode("ascii")
                        row["rip"].append(ref)
                    elif op.type==x.X86_OP_IMM and ins.mnemonic in ("call","jmp"):
                        target=op.imm
                        row["branch"]={"va":hex(target),"name":names.get(target)}
                bodies[owner].append(row)
    assert set(bodies)==set(TARGETS),[TARGETS[v] for v in set(TARGETS)-set(bodies)]
    a.out.mkdir(parents=True,exist_ok=True)
    index=[]
    for owner,rows in sorted(bodies.items()):
        assert int(rows[0]["va"],16)==owner
        name=TARGETS[owner]
        (a.out/(name+".json")).write_text(json.dumps(rows,indent=2)+"\n")
        (a.out/(name+".asm")).write_text("\n".join(
            f'{r["va"]} {r["bytes"]:24} {r["op"]:10} {r["args"]} ; {json.dumps({"rip":r["rip"],"branch":r.get("branch")},separators=(",",":"))}' for r in rows)+"\n")
        calls=sorted({r["branch"]["va"] for r in rows if r["op"]=="call" and "branch" in r})
        index.append({"name":name,"start":hex(owner),"instructions":len(rows),
                      "direct_calls":[{"va":v,"name":names.get(int(v,16))} for v in calls]})
        print(name,hex(owner),len(rows),"instructions")
    (a.out/"index.json").write_text(json.dumps(index,indent=2)+"\n")
if __name__=="__main__":main()
