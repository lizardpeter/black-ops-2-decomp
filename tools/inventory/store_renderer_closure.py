#!/usr/bin/env python3
"""Build a focused Store renderer call graph and evidence bundle.

Direct call and tail-call edges are decoded from the hash-pinned PE at Ghidra
instruction boundaries; indirect calls are recorded but never guessed.
Each root receives its own depth budget to avoid turning the text subsystem
into an unbounded whole-game graph. Source is evidence, not compiled C++.
"""
import argparse, collections, hashlib, json, re
from pathlib import Path
import capstone as cs
from capstone import x86_const as x
import pefile
from store_dvar_next import ROOT, DIGEST, HEADER, INSN
ROOTS={
  0x140732480:("text_2d",2),0x140734690:("teletype",2),
  0x1406af8f0:("lui_text",3),0x140735340:("text_3d",2),
  0x14073d680:("tess_submit",3),0x14071c880:("text_command",2),
  0x1403fbaf0:("font_assets",2),0x1400f27a0:("text_substitute",2),
  0x1407203b0:("renderer_init",2),0x140215300:("zone_selection",1),
  0x14071a7a0:("text_width",2),0x1401f65e0:("screen_placement",2),
}
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("exe",type=Path);ap.add_argument("out",type=Path)
    ap.add_argument("--limit",type=int,default=384)
    a=ap.parse_args();raw=a.exe.read_bytes();assert hashlib.sha256(raw).hexdigest()==DIGEST
    pe=pefile.PE(data=raw,fast_load=True);base=pe.OPTIONAL_HEADER.ImageBase
    md=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_64);md.detail=True
    addresses=collections.defaultdict(list);reverse=collections.defaultdict(set)
    # Only lines with direct call/jmp targets need Capstone during discovery.
    for shard in sorted((ROOT/"analysis/t6mp_msstore/disasm/_unattributed").glob("*.asm")):
        owner=None
        for line in shard.open(encoding="utf-8"):
            h=HEADER.match(line)
            if h:owner=int(h[1],16);continue
            m=INSN.match(line)
            if m is None or owner is None:continue
            va=int(m[1],16);addresses[owner].append(va)
            if re.search(r"\b(?:CALL|JMP)\b",line,re.I):
                ins=next(md.disasm(pe.get_data(va-base,15),va,count=1),None)
                if ins and ins.mnemonic in ("call","jmp") and ins.operands and ins.operands[0].type==x.X86_OP_IMM:
                    reverse[ins.operands[0].imm].add(owner)
    assert set(ROOTS)<=set(addresses)
    cache={}
    def body(va):
        if va in cache:return cache[va]
        rows=[]
        for pc in addresses[va]:
            ins=next(md.disasm(pe.get_data(pc-base,15),pc,count=1),None)
            if not ins:continue
            refs=[]
            for op in ins.operands:
                if op.type!=x.X86_OP_MEM or op.mem.base!=x.X86_REG_RIP:continue
                target=pc+ins.size+op.mem.disp
                sec=pe.get_section_by_rva(target-base)
                item={"va":hex(target),"size":op.size}
                if sec is not None and not sec.Characteristics&0x80000000:
                    item["bytes"]=pe.get_data(target-base,op.size).hex()
                    if ins.mnemonic=="lea":
                        s=pe.get_data(target-base,120).split(b"\0",1)[0]
                        if s and all(32<=b<127 for b in s):item["string"]=s.decode("ascii")
                refs.append(item)
            direct=ins.operands[0].imm if ins.mnemonic in ("call","jmp") and ins.operands and ins.operands[0].type==x.X86_OP_IMM else None
            rows.append({"va":hex(pc),"op":ins.mnemonic,"args":ins.op_str,"bytes":ins.bytes.hex(),"rip":refs,"target":hex(direct) if direct is not None else None})
        cache[va]=rows;return rows
    chosen=collections.defaultdict(dict);pending=collections.deque()
    for root,(name,depth) in ROOTS.items():pending.append((root,name,0,depth))
    # Direct parents of the two backend loops and of the world-space path
    # identify command dispatch and public wrappers without relying on names.
    for root in (0x140732480,0x140734690,0x140735340,0x14071c880):
        for parent in sorted(reverse[root]):
            if parent!=root and parent in addresses and len(addresses[parent])<=1000:
                pending.append((parent,"caller_"+hex(root),0,1))
    while pending:
        va,name,depth,maxdepth=pending.popleft()
        if name in chosen[va] and chosen[va][name]<=depth:continue
        if not chosen[va] and sum(bool(v) for v in chosen.values())>=a.limit:continue
        chosen[va][name]=depth
        if depth>=maxdepth:continue
        for row in body(va):
            if row["target"] is None:continue
            t=int(row["target"],16)
            if t!=va and t in addresses:pending.append((t,name,depth+1,maxdepth))
    chosen={k:v for k,v in chosen.items() if v}
    a.out.mkdir(parents=True,exist_ok=True)
    pseudo={}
    for shard in sorted((ROOT/"analysis/t6mp_msstore/pseudocode/_unattributed").glob("*.c")):
        owner=None;buf=[]
        for line in shard.open(encoding="utf-8"):
            h=re.match(r"^//\s+([0-9a-fA-F]{9,16})\s+",line)
            if h:
                if owner in chosen:pseudo[owner]="".join(buf)
                owner=int(h[1],16);buf=[]
            if owner in chosen:buf.append(line)
        if owner in chosen:pseudo[owner]="".join(buf)
    index=[]
    for va,roles in sorted(chosen.items()):
        rows=body(va);stem=f"{va:x}"
        (a.out/(stem+".json")).write_text(json.dumps(rows,indent=2)+"\n")
        (a.out/(stem+".asm")).write_text("\n".join(f'{r["va"]} {r["bytes"]:24} {r["op"]:10} {r["args"]} ; {json.dumps(r["rip"])}' for r in rows)+"\n")
        if va in pseudo:(a.out/(stem+".c")).write_text(pseudo[va])
        targets=sorted({r["target"] for r in rows if r["target"] and int(r["target"],16) in addresses})
        indirect=[r["va"] for r in rows if r["op"]=="call" and r["target"] is None]
        index.append({"va":hex(va),"roles":roles,"instructions":len(rows),"targets":targets,"indirect_calls":indirect,"pseudocode":va in pseudo})
    (a.out/"index.json").write_text(json.dumps(index,indent=2)+"\n")
    print("Unique bodies:",len(index),"instructions:",sum(r["instructions"] for r in index),"existing pseudocode:",len(pseudo))
    print("Root callers:")
    for root in ROOTS:print(hex(root),len(reverse[root])," ".join(hex(v) for v in sorted(reverse[root]) if v in chosen))
    print("Role counts:",collections.Counter(k for item in index for k in item["roles"]))
if __name__=="__main__":main()
