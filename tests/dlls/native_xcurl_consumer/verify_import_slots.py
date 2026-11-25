#!/usr/bin/env python3
"""Check the optional Windows x64 import-shaped static archive in a real PE.
No original executables are patched or executed.
"""
import json
import re
import struct
import sys
from pathlib import Path
import pefile
ROOT=Path(__file__).resolve().parents[3]
def main():
    exe,map_path=map(Path,sys.argv[1:3])
    expected=set(json.loads((ROOT/"docs/xcurl-store-callers.json").read_text())["imports"])
    image=pefile.PE(str(exe))
    assert image.FILE_HEADER.Machine==0x8664
    base=image.OPTIONAL_HEADER.ImageBase
    deps={d.dll.decode().lower() for d in image.DIRECTORY_ENTRY_IMPORT}
    assert "winhttp.dll" in deps and not any("xcurl" in d or "libcurl" in d for d in deps),deps
    addresses={}
    wanted=expected|{"__imp_"+n for n in expected}
    for line in map_path.read_text(errors="replace").splitlines():
        match=re.match(r"\s*[0-9A-Fa-f]+:[0-9A-Fa-f]+\s+(\S+)\s+([0-9A-Fa-f]+)\s+(.*)",line)
        if not match or match[1] not in wanted:continue
        name,va,origin=match.groups()
        assert name not in addresses,(name,"duplicate")
        addresses[name]=(int(va,16),origin)
    assert addresses.keys()==wanted,(wanted-addresses.keys())
    relocs={entry.rva for block in image.DIRECTORY_ENTRY_BASERELOC for entry in block.entries if entry.type==10}
    for name in sorted(expected):
        target,origin=addresses[name]
        slot,slot_origin=addresses["__imp_"+name]
        assert "t6_native_xcurl_research" in origin,(name,origin)
        assert "t6_native_xcurl_import_abi" in slot_origin,(name,slot_origin)
        code=image.get_section_by_rva(target-base)
        data=image.get_section_by_rva(slot-base)
        assert code and code.Characteristics&0x20000000,(name,"not executable code")
        assert data and not data.Characteristics&0x20000000 and data.Characteristics&0x40000000,(name,"not readable data")
        assert struct.unpack("<Q",image.get_data(slot-base,8))[0]==target,(name,"wrong target pointer")
        assert slot-base in relocs,(name,"missing x64 base relocation")
        print(name,hex(slot-base),"->",hex(target-base))
    print("XCurl native import-slot PE audit PASS:",len(expected),"exact static pointer slots, all relocatable")
if __name__=="__main__":main()
