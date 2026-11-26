#!/usr/bin/env python3
"""Extract Store font entrypoints and direct dependencies; discover glyph candidates."""
import argparse, hashlib, json, re
from collections import defaultdict
from pathlib import Path
import capstone as cs
from capstone import x86_const as x
import pefile
from store_dvar_next import ROOT,DIGEST,HEADER,INSN

SEEDS={0x1403FCD20:"UI_DefaultFont",0x1403FCE40:"UI_GetFontHandle",
       0x14071A780:"R_RegisterFont",0x14071A7A0:"R_TextWidth",
       0x1403FC6D0:"UI_DrawTextPadding",0x140215300:"DB_ResolutionZoneNames",
       0x1407203B0:"RendererInit",0x1403FBAF0:"UI_AssetCache",
       0x14071C880:"AddBaseDrawTextCmd",0x1400F2040:"ConvertColor",0x140723340:"FontIconLookup"}
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("exe",type=Path);p.add_argument("out",type=Path);a=p.parse_args()
    raw=a.exe.read_bytes();assert hashlib.sha256(raw).hexdigest()==DIGEST
    pe=pefile.PE(data=raw,fast_load=True);base=pe.OPTIONAL_HEADER.ImageBase
    md=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_64);md.detail=True
    addresses=defaultdict(list);glyph=set();incoming=set()
    for shard in sorted((ROOT/"analysis/t6mp_msstore/disasm/_unattributed").glob("*.asm")):
        owner=None;tokens=set()
        with shard.open(encoding="utf-8") as f:
            for line in f:
                h=HEADER.match(line)
                if h:
                    if owner is not None and {"0x5f","0x60","0x20"}<=tokens:glyph.add(owner)
                    owner=int(h[1],16);tokens=set();continue
                m=INSN.match(line)
                if not m or owner is None:continue
                addresses[owner].append(int(m[1],16))
                if re.search(r"\bCALL\s+(?:0x)?0*14071a6e0\b",line,re.I):incoming.add(owner)
                tokens.update(re.findall(r"(?<![0-9a-f])0x(?:5f|60|20)(?![0-9a-f])",line.lower()))
            if owner is not None and {"0x5f","0x60","0x20"}<=tokens:glyph.add(owner)
    assert set(SEEDS)<=set(addresses)
    def body(owner):
        rows=[]
        for va in addresses[owner]:
            ins=next(md.disasm(pe.get_data(va-base,15),va,count=1),None)
            if ins is None:continue
            refs=[]
            for op in ins.operands:
                if op.type==x.X86_OP_MEM and op.mem.base==x.X86_REG_RIP:
                    target=va+ins.size+op.mem.disp
                    entry={"va":hex(target),"size":op.size}
                    sec=pe.get_section_by_rva(target-base)
                    if sec is not None and not sec.Characteristics&0x80000000:
                        entry["bytes"]=pe.get_data(target-base,op.size).hex()
                        if ins.mnemonic=="lea":
                            s=pe.get_data(target-base,120).split(b"\0",1)[0]
                            if s and all(32<=b<127 for b in s):entry["string"]=s.decode("ascii")
                    refs.append(entry)
            target=None
            if ins.mnemonic in ("call","jmp") and ins.operands and ins.operands[0].type==x.X86_OP_IMM:
                target=ins.operands[0].imm
            rows.append({"va":hex(va),"op":ins.mnemonic,"args":ins.op_str,"bytes":ins.bytes.hex(),"rip":refs,"target":hex(target) if target else None})
        return rows
    chosen=dict(SEEDS)
    # Only discover direct calls from font/draw entrypoints, not the entire renderer initialization tree.
    for owner in list(SEEDS)[:5]+[0x14071C880]:
        for row in body(owner):
            if not row["target"]:continue
            t=int(row["target"],16)
            if t!=owner and t in addresses:chosen.setdefault(t,"dependency_"+hex(t))
    # The x86 reference uses ASCII 0x20..0x7f and starts its non-ASCII search at 0x60.
    # This is candidate evidence only, never a semantic identity assignment.
    for owner in incoming:chosen.setdefault(owner,"glyph_caller_"+hex(owner))
    for owner in glyph:
        if len(addresses[owner])<=150:chosen.setdefault(owner,"glyph_candidate_"+hex(owner))
    a.out.mkdir(parents=True,exist_ok=True);index=[]
    for owner,name in sorted(chosen.items()):
        rows=body(owner)
        (a.out/(f"{owner:x}.json")).write_text(json.dumps(rows,indent=2)+"\n")
        (a.out/(f"{owner:x}.asm")).write_text("\n".join(f'{r["va"]} {r["bytes"]:24} {r["op"]:10} {r["args"]} ; {json.dumps(r["rip"])}' for r in rows)+"\n")
        index.append({"va":hex(owner),"name":name,"instructions":len(rows),"targets":sorted({r["target"] for r in rows if r["target"] and r["op"]=="call"})})
        print(hex(owner),name,len(rows),"instructions")
    (a.out/"index.json").write_text(json.dumps(index,indent=2)+"\n")
if __name__=="__main__":main()
