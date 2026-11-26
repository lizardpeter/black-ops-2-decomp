#!/usr/bin/env python3
"""Store MP HUD resolution evidence, from hash-pinned PE and instruction ownership."""
import argparse, hashlib, json, re, struct
from collections import defaultdict
from pathlib import Path
import capstone as cs
from capstone import x86_const as x
import pefile
from store_dvar_next import ROOT, DIGEST, HEADER, INSN

GLOBALS={0x1427E71D0:"r_fontResolution",0x1427E6F88:"r_resolution",
         0x140E6287C:"hudScale",0x1427F1B64:"captureScaleX",0x1427F1B68:"captureScaleY"}
STRINGS=re.compile(rb"(?:fonts/[^\x00\r\n]{1,100}|code_post_gfx[^\x00\r\n]{0,40}|_%d%s|(?:extra(?:big|small)|big|small|normal|bold|console|dist)font)\x00",re.I)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("exe",type=Path);p.add_argument("out",type=Path);a=p.parse_args()
    raw=a.exe.read_bytes();assert hashlib.sha256(raw).hexdigest()==DIGEST
    pe=pefile.PE(data=raw,fast_load=True);base=pe.OPTIONAL_HEADER.ImageBase
    targets=dict(GLOBALS)
    for sec in pe.sections:
        if sec.Characteristics&0x20000000:continue
        for m in STRINGS.finditer(sec.get_data()):
            s=m.group()[:-1]
            if all(32<=c<127 for c in s):
                targets[base+sec.VirtualAddress+m.start()]=s.decode("ascii")
    md=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_64);md.detail=True
    positions=set();candidates={v-d for v in targets for d in range(9)}
    for sec in pe.sections:
        if not sec.Characteristics&0x20000000:continue
        b=sec.get_data();va=base+sec.VirtualAddress
        for shift in range(4):
            end=shift+((len(b)-shift)//4)*4
            for i,(disp,) in enumerate(struct.iter_unpack("<i",memoryview(b)[shift:end])):
                pos=va+shift+4*i
                if pos+4+disp in candidates:positions.update(range(pos-11,pos+1))
    shards=sorted((ROOT/"analysis/t6mp_msstore/disasm/_unattributed").glob("*.asm"));assert len(shards)==9
    def decode(va):return next(md.disasm(pe.get_data(va-base,15),va,count=1),None)
    def refs(ins):
        return [(ins.address+ins.size+op.mem.disp,op.access) for op in ins.operands if op.type==x.X86_OP_MEM and op.mem.base==x.X86_REG_RIP]
    hits=[];owners=set()
    for shard in shards:
        owner=None
        with shard.open(encoding="utf-8") as f:
            for line in f:
                h=HEADER.match(line)
                if h:owner=int(h[1],16);continue
                m=INSN.match(line)
                if not m or owner is None:continue
                va=int(m[1],16)
                if va not in positions:continue
                ins=decode(va)
                if ins is None:continue
                for target,access in refs(ins):
                    if target not in targets:continue
                    hits.append(dict(owner=hex(owner),va=hex(va),target=hex(target),name=targets[target],access=access,op=ins.mnemonic,args=ins.op_str))
                    if target in GLOBALS or targets[target].startswith("fonts/") or targets[target]=="_%d%s":owners.add(owner)
    bodies=defaultdict(list)
    for shard in shards:
        owner=None
        with shard.open(encoding="utf-8") as f:
            for line in f:
                h=HEADER.match(line)
                if h:owner=int(h[1],16);continue
                if owner not in owners:continue
                m=INSN.match(line)
                if not m:continue
                va=int(m[1],16);ins=decode(va)
                if ins is None:continue
                annotations=[]
                for target,_ in refs(ins):
                    if target in targets:annotations.append(targets[target])
                    else:
                        sec=pe.get_section_by_rva(target-base)
                        if sec is not None and not sec.Characteristics&0x80000000 and ins.mnemonic=="lea":
                            b=pe.get_data(target-base,100).split(b"\0",1)[0]
                            if b and all(32<=c<127 for c in b):annotations.append(b.decode("ascii"))
                bodies[owner].append(f"{va:011x} {ins.bytes.hex():24} {ins.mnemonic:10} {ins.op_str} ; {' | '.join(annotations)}")
    a.out.mkdir(parents=True,exist_ok=True)
    (a.out/"references.json").write_text(json.dumps(hits,indent=2)+"\n")
    (a.out/"strings.json").write_text(json.dumps({hex(k):v for k,v in targets.items()},indent=2)+"\n")
    for owner,rows in bodies.items():(a.out/(f"{owner:x}.asm")).write_text("\n".join(rows)+"\n")
    for h in hits:
        if h["name"] in ("r_fontResolution","r_resolution","_%d%s") or h["name"].startswith("fonts/"):
            print(h["owner"],h["va"],h["name"],h["op"],h["args"])
    print(f"{len(hits)} exact RIP references, {len(bodies)} enclosing instruction bodies")
    assert any(h["name"]=="r_fontResolution" for h in hits)
if __name__=="__main__":main()
