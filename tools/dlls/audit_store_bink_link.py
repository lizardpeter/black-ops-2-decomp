#!/usr/bin/env python3
"""Audit a source-only Bink validation module against pinned Store client PE imports.
Read-only: no original is executed, modified, or uploaded.
"""
import argparse
import hashlib
import json
from pathlib import Path
import pefile
ROOT=Path(__file__).resolve().parents[2]
def load(path):
    pe=pefile.PE(str(path))
    assert pe.FILE_HEADER.Machine==0x8664 and pe.OPTIONAL_HEADER.Magic==0x20B,(path,"not Windows x64")
    return pe
def imports(pe):
    result={}
    for d in getattr(pe,"DIRECTORY_ENTRY_IMPORT",[]):
        result[d.dll.decode().lower()]=sorted(i.name.decode() if i.name else "#"+str(i.ordinal) for i in d.imports)
    return result
def audit(module,static_client,dynamic_client,originals):
    dll=load(module)
    assert dll.FILE_HEADER.Characteristics&0x2000
    exports={e.name.decode():e for e in dll.DIRECTORY_ENTRY_EXPORT.symbols if e.name}
    expected=set(json.loads((ROOT/"docs/bink-store-mp-callers.json").read_text())["call_sites"]) | {"BinkOpen"}
    assert len(expected)==19
    assert set(exports)==expected,(set(exports)-expected,expected-set(exports))
    for name,e in exports.items():
        assert not e.forwarder,(name,"forwarded export")
        sec=dll.get_section_by_rva(e.address)
        assert sec and sec.Characteristics&0x20000000,(name,"not executable source export")
    dependencies=imports(dll)
    assert not any("bink" in d or "xcurl" in d or "avcodec" in d or "ffmpeg" in d for d in dependencies)
    for client,dynamic in ((static_client,False),(dynamic_client,True)):
        deps=imports(load(client))
        if dynamic:assert set(deps["binkw64.dll"])==expected,(client,deps.get("binkw64.dll"))
        else:assert not any("bink" in d or "xcurl" in d for d in deps),(client,deps)
    result={"module":module.name,"sha256":hashlib.sha256(module.read_bytes()).hexdigest(),
            "exports":sorted(exports),"module_imports":dependencies,"originals":{}}
    for name,path in originals.items():
        pe=load(path)
        actual=imports(pe)["binkw64.dll"]
        assert set(actual)==expected,(name,actual)
        result["originals"][name]={"sha256":hashlib.sha256(path.read_bytes()).hexdigest(),"imports":actual}
    return result
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--module",type=Path,required=True)
    p.add_argument("--static-client",type=Path,required=True)
    p.add_argument("--dynamic-client",type=Path,required=True)
    p.add_argument("--originals",type=Path,required=True)
    p.add_argument("--out",type=Path,required=True)
    a=p.parse_args()
    result=audit(a.module,a.static_client,a.dynamic_client,{n:a.originals/(n+".exe") for n in
        ("t6mp_msstore","t6sp_msstore","t6zm_msstore")})
    a.out.parent.mkdir(parents=True,exist_ok=True)
    a.out.write_text(json.dumps(result,indent=2)+"\n")
    print(json.dumps(result,indent=2))
    print("All three Store import contracts PASS: 19 native exports, no forwarders or vendor codec dependency")
if __name__=="__main__":main()
