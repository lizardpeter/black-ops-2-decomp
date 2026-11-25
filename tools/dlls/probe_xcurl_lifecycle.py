#!/usr/bin/env python3
"""Read-only disassembly of remaining Store XCurl lifecycle and callback targets."""
import hashlib
import sys
from pathlib import Path
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_MEM, CS_OP_IMM
from capstone.x86_const import X86_REG_RIP

SHA = "65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551"
TARGETS = (0x19C54, 0x19B8C, 0x198E4, 0xA128, 0x14AC0, 0x1738C, 0x1CFD0, 0x1ADA4, 0x16960, 0x16B60, 0x8BC0, 0x8C50)
def main():
    path = Path(sys.argv[1])
    assert hashlib.sha256(path.read_bytes()).hexdigest() == SHA
    pe = pefile.PE(str(path))
    base = pe.OPTIONAL_HEADER.ImageBase
    ranges = {e.struct.BeginAddress: e.struct.EndAddress for e in pe.DIRECTORY_ENTRY_EXCEPTION}
    imports = {i.address: (d.dll.decode(), i.name.decode() if i.name else str(i.ordinal))
               for d in pe.DIRECTORY_ENTRY_IMPORT for i in d.imports}
    md = Cs(CS_ARCH_X86, CS_MODE_64)
    md.detail = True
    def show(rva):
        end = ranges.get(rva)
        if end is None:
            print(f"RVA {rva:05X} has no exact unwind record; bounded leaf inspection")
            end = min((s for s in ranges if s > rva), default=rva+128)
            end = min(end, rva+128)
        print(f"FUNCTION {rva:05X}..{end:05X}", flush=True)
        for i in md.disasm(pe.get_data(rva, end-rva), base+rva):
            annotation = []
            for o in i.operands:
                if o.type == CS_OP_MEM and o.mem.base == X86_REG_RIP:
                    addr = i.address+i.size+o.mem.disp
                    if addr in imports:
                        annotation.append("!".join(imports[addr]))
                    else:
                        annotation.append(f"data={addr-base:05X}")
                if o.type == CS_OP_IMM and i.mnemonic.startswith(("call","j")) and base <= o.imm < base+pe.OPTIONAL_HEADER.SizeOfImage:
                    annotation.append(f"target={o.imm-base:05X}")
            print(f"{i.address-base:05X} {i.mnemonic:8} {i.op_str:40} {' '.join(annotation)}")
    for rva in TARGETS: show(rva)
    for rva in (0x21B40,0x20D10,0x20D38,0x20D70,0x20DA8,0x20E10,0x20E50,0x20E78,0x20EB8,0x20EF0,0x20F20,0x20F48,0x20F60):
        data=pe.get_data(rva,256)
        print(f'DATA {rva:05X}: {data[:80].hex()} ASCII {data.split(bytes([0]),1)[0]!r}')
    print("PUBLIC EXPORT RVAS")
    for e in pe.DIRECTORY_ENTRY_EXPORT.symbols:
        if e.name and e.name.startswith(b"curl_"):
            print(f"{e.name.decode()} {e.address:05X}")
if __name__ == "__main__":
    main()
