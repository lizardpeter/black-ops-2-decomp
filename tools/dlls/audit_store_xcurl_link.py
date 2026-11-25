#!/usr/bin/env python3
"""Audit actual Store import/option contracts and the native static link map.
Original executables are hash-pinned R2 inputs, read only and never executed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import tomllib
import pefile
from probe_xcurl_setopt import load, imported, call_evidence

ROOT=Path(__file__).resolve().parents[2]
def native_symbols(exe,linkmap,expected):
    pe=load(exe)
    deps={d.dll.decode().lower() for d in pe.DIRECTORY_ENTRY_IMPORT}
    assert "winhttp.dll" in deps
    assert not any("xcurl" in d or "libcurl" in d for d in deps),deps
    symbols={}
    for line in linkmap.read_text(errors="replace").splitlines():
        m=re.match(r"\s*[0-9A-Fa-f]+:[0-9A-Fa-f]+\s+(\S+)\s+([0-9A-Fa-f]+)\s+(.*)",line)
        if not m or m[1] not in expected:continue
        name,addr,origin=m.groups()
        assert "t6_native_xcurl_research" in origin,(name,origin)
        rva=int(addr,16)-pe.OPTIONAL_HEADER.ImageBase
        section=pe.get_section_by_rva(rva)
        assert section and section.Characteristics&0x20000000,(name,hex(rva))
        assert name not in symbols,(name,"duplicate static symbol")
        symbols[name]={"rva":hex(rva),"origin":origin.strip()}
    assert set(symbols)==expected,(expected-set(symbols),symbols)
    return {"sha256":hashlib.sha256(exe.read_bytes()).hexdigest(),"imports":sorted(deps),"symbols":symbols}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--exe",type=Path,required=True);p.add_argument("--map",type=Path,required=True)
    p.add_argument("--originals",type=Path,required=True);p.add_argument("--out",type=Path,required=True)
    args=p.parse_args()
    manifest=json.loads((ROOT/"docs/xcurl-store-callers.json").read_text())
    expected=set(manifest["imports"]);assert len(expected)==16
    cfg=tomllib.loads((ROOT/"target.toml").read_text())
    source=ROOT/"reconstructed_dlls/xcurl"
    abi=(source/"game_api.cpp").read_text()
    setopt=abi[abi.index('extern "C" int curl_easy_setopt'):abi.index('extern "C" int curl_easy_getinfo')]
    supported_setopt={int(s,0) for s in re.findall(r"case\s+(0x[0-9a-fA-F]+|\d+)\s*:",setopt)}
    dispatch=(source/"option_dispatch.cpp").read_text()
    implemented_setopt={int(s,0) for s in re.findall(r"case\s+(0x[0-9a-fA-F]+|\d+)\s*:",dispatch)}
    supported_getinfo={int(s,0) for s in re.findall(r"case\s+(0x[0-9a-fA-F]+|\d+)\s*:",(source/"getinfo_unlocked.cpp").read_text())}
    result={"native":native_symbols(args.exe,args.map,expected),"originals":{},"unresolved":[]}
    for build in manifest["builds"]:
        name=build["build"];f=args.originals/(name+".exe")
        pinned=next(b["exe"]["sha256"] for b in cfg["build"] if b["id"]==name)
        sha=hashlib.sha256(f.read_bytes()).hexdigest();assert sha==pinned,(name,sha)
        pe=load(f);iat=imported(pe,"xcurl.dll");assert set(iat.values())==expected,(name,iat)
        entry={"sha256":sha,"imports":sorted(iat.values()),"setopt":[],"getinfo":[]}
        for kind,supported in (("setopt",supported_setopt&implemented_setopt),("getinfo",supported_getinfo)):
            for site in build[kind]:
                rec=call_evidence(pe,int(site["call_rva"],16),"curl_easy_"+kind,iat)
                values=rec["options"]
                if values is None:
                    result["unresolved"].append({"build":name,"kind":kind,"rva":rec["rva"]})
                    print("UNRESOLVED",name,rec["rva"],"\n"+"\n".join(rec["instructions"]),flush=True)
                else:
                    assert set(values)<=supported,(name,kind,rec["rva"],values)
                    if "option" in site:assert site["option"] in values,(name,site,rec)
                    if "options" in site:assert site["options"]==values,(name,site,rec)
                entry[kind].append({"rva":rec["rva"],"options":values})
        result["originals"][name]=entry
        print(name,"16 imports matched", {k:[v["options"] for v in entry[k]] for k in ("setopt","getinfo")},flush=True)
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(result,indent=2)+"\n")
    print("All 16 source-linked x64 symbols confirmed in executable sections; no vendor XCurl or libcurl imports.")
    print("Original caller option recovery:",60-len(result["unresolved"]),"/ 60; unresolved:",result["unresolved"],flush=True)
if __name__=="__main__":main()
