#!/usr/bin/env python3
"""Extract Store MP FOV references and core dvar bodies from the pinned PE.

Ghidra instruction starts are used to reject false positives in a byte scan.
RIP-relative references are exact; indirect aliases are not covered.
"""
import argparse
import hashlib
import json
import re
import struct
from collections import defaultdict
from pathlib import Path

import capstone as cs
from capstone import x86_const as x
import pefile

ROOT = Path(__file__).resolve().parents[2]
DIGEST = "cf80b8058bce7e00e4c61ff3d673308c1fcd8061f18fe66710803538eab4bc01"
FOV = {0x140E4F8E8:"cg_fov",0x140D78658:"cg_fovScale",0x140D78648:"cg_fovMin",
       0x140D78638:"cg_fovExtraCam",0x142161688:"cg_fov_default",0x142161678:"cg_fov_default_thirdperson"}
CORE = {0x14046B300:"SetVariant",0x14046B150:"SetLatched",0x14046CEF0:"SetFloat",
        0x14046B950:"GetInt",0x14046B830:"FindByHash",0x14046B8B0:"FindByName",
        0x14046C1A0:"RegisterNew",0x14046BF10:"Reregister",0x14061BAF0:"SharedGameplay",
        0x14046B010:"ValuesEqual",0x14046AD80:"Constrain",0x14046ABD0:"Parse",
        0x14046A6A0:"Format",0x1400C1B90:"FovCamera",0x140101410:"FovPostRegistration",
        0x1400C12F0:"CameraWeaponFov",0x1400C1A00:"CameraAdjustFov",0x1400C2060:"CameraNextView",
        0x1400F9280:"FovDefaultSelect",0x1400F92F0:"FovDefaultUpdate",0x1400F93E0:"FovConsumerA",
        0x1400F9470:"FovConsumerB",0x1400F98D0:"FovDefaultConsumer",0x14016CE80:"ExtraCameraFov",
        0x14016C510:"MainView",0x1401721E0:"ExtraViewWrapper",0x1400C2740:"VehicleGunner",
        0x1400C1240:"ExtraMode",0x140173ED0:"ExtraUpdate",0x14016BDA0:"SpecialView",
        0x140100FC0:"LinkedView",0x1400C3AE0:"ExtraMode6A",0x1400C3E10:"ExtraMode6B",
        0x1400C3390:"ExtraMode7",0x140464D00:"AnglesToAxis",0x14065B060:"AdsTransitionA",0x14065B120:"AdsTransitionB",
        0x1400C0E20:"MainCameraMode",0x14016B450:"LastStandSway",0x14016BA40:"FirstPersonView",
        0x14016A940:"ThirdPersonView",0x14016C380:"CameraVelocity",0x1400BF940:"CameraTween",
        0x1400C7540:"CameraShake",0x140157BC0:"CameraPerturb",0x14016BFB0:"TurretView",
        0x1400C4490:"VehicleView",0x1400C5FA0:"VehicleThirdPerson",0x1400C66D0:"VehicleOffset",
        0x1400C3950:"RemoteEyes",0x140621C10:"LinkedAngleTransform",0x1400DE970:"TagAxis",
        0x140462D70:"AxisToAngles",0x1400C72B0:"UpdateShake",0x140660B40:"BobAngles",0x1406602A0:"VerticalBob",0x140660470:"HorizontalBob",0x140472FE0:"OffsetOrigin",0x14016A5D0:"ChaseView",0x1400C69F0:"ThirdPersonTrace",0x14038BAD0:"WaterHeight"}
HEADER = re.compile(r"^; ([0-9a-fA-F]+)\s")
INSN = re.compile(r"^\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+")

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("exe",type=Path)
    p.add_argument("out",type=Path)
    a=p.parse_args()
    raw=a.exe.read_bytes()
    if hashlib.sha256(raw).hexdigest()!=DIGEST:
        raise ValueError("wrong original")
    pe=pefile.PE(data=raw,fast_load=True)
    base=pe.OPTIONAL_HEADER.ImageBase
    assert base==0x140000000
    md=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_64)
    md.detail=True
    def read(va,n):
        return pe.get_data(va-base,n)
    def annotate(ins):
        refs=[]
        for op in ins.operands:
            if op.type==x.X86_OP_MEM and op.mem.base==x.X86_REG_RIP:
                va=ins.address+ins.size+op.mem.disp
                r={"va":hex(va),"size":op.size,"access":op.access}
                sec=pe.get_section_by_rva(va-base)
                if sec is not None and not sec.Characteristics&0x80000000:
                    b=read(va,op.size)
                    r["bytes"]=b.hex()
                    if ins.mnemonic=="lea":
                        buf=read(va,256).split(b"\0",1)[0]
                        if buf and all(32<=c<127 for c in buf): r["string"]=buf.decode("ascii")
                refs.append(r)
        return {"va":hex(ins.address),"bytes":ins.bytes.hex(),"op":ins.mnemonic,"args":ins.op_str,"rip":refs}
    # Find plausible displacement fields by four aligned passes; a RIP-relative
    # displacement is signed 32-bit and the following immediate is at most 8 bytes.
    positions=set()
    candidates={v-d for v in FOV for d in range(9)}
    for sec in pe.sections:
        if not sec.Characteristics&0x20000000: continue
        b=sec.get_data()
        va=base+sec.VirtualAddress
        for shift in range(4):
            end=shift+((len(b)-shift)//4)*4
            for i,(disp,) in enumerate(struct.iter_unpack("<i",memoryview(b)[shift:end])):
                pos=va+shift+4*i
                if pos+4+disp in candidates:
                    positions.update(range(pos-11,pos+1))
    shards=sorted((ROOT/"analysis/t6mp_msstore/disasm/_unattributed").glob("*.asm"))
    assert len(shards)==9
    owners=defaultdict(list)
    hits=[]
    starts=set()
    for shard in shards:
        owner=None
        with shard.open(encoding="utf-8") as f:
            for line in f:
                header=HEADER.match(line)
                if header:
                    owner=int(header[1],16)
                    continue
                m=INSN.match(line)
                if not m or owner is None: continue
                va=int(m[1],16)
                if owner not in CORE and va not in positions: continue
                ins=next(md.disasm(read(va,15),va,count=1),None)
                if ins is None: continue
                if owner in CORE:
                    owners[owner].append(annotate(ins))
                    starts.add(owner)
                if va in positions:
                    row=annotate(ins)
                    matched=[FOV[int(r["va"],16)] for r in row["rip"] if int(r["va"],16) in FOV]
                    if matched:
                        hits.append({"owner":hex(owner),"dvars":matched,**row})
    assert starts==set(CORE),(starts,set(CORE))
    assert any("cg_fovScale" in h["dvars"] for h in hits)
    a.out.mkdir(parents=True,exist_ok=True)
    (a.out/"fov-references.json").write_text(json.dumps(hits,indent=2)+"\n")
    for va,rows in owners.items():
        assert rows[0]["va"]==hex(va),(va,rows[0])
        (a.out/(CORE[va]+".json")).write_text(json.dumps(rows,indent=2)+"\n")
        (a.out/(CORE[va]+".asm")).write_text("\n".join(
            f'{r["va"]} {r["bytes"]:24} {r["op"]:10} {r["args"]} ; {json.dumps(r["rip"],separators=(",",":"))}' for r in rows)+"\n")
    for name in FOV.values():
        selected=[h for h in hits if name in h["dvars"]]
        print(name, len(selected), "direct RIP references:", sorted({h["owner"] for h in selected}))
    print("Core body instruction counts:",{CORE[va]:len(rows) for va,rows in owners.items()})
    for va in (0x14046CEF0,0x14046B950,0x14046B150):
        print(CORE[va])
        for r in owners[va]:
            print(r["va"],r["op"],r["args"],r["rip"])

if __name__=="__main__":
    main()
