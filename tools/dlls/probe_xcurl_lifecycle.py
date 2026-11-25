#!/usr/bin/env python3
"""Read-only disassembly of remaining Store XCurl lifecycle and callback targets."""
import hashlib
import sys
from pathlib import Path
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_MEM, CS_OP_IMM
from capstone.x86_const import X86_REG_RIP

SHA = "65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551"
TARGETS = (0x8018, 0x83D8, 0x8970, 0x1230, 0x1DF38, 0x7A30, 0x7DF4, 0x80B8)
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
    for rva in (0x282BC,0x28030,0x28070,0x205B8,0x23500,0x23530,0x233E0,0x232E0,0x23300,0x2329C,0x2332C,0x20658,0x20660,0x206B0,0x206C8,0x23304,0x23388,0x2338C,0x23390,0x233A0,0x233A8,0x233AC,0x233B0,0x233B4,0x233B8,0x232DC,0x21B40,0x20D10,0x20D38,0x20D70,0x20DA8,0x20E10,0x20E50,0x20E78,0x20EB8,0x20EF0,0x20F20,0x20F48,0x20F60):
        data=pe.get_data(rva,256)
        print(f'DATA {rva:05X}: {data[:80].hex()} ASCII {data.split(bytes([0]),1)[0]!r}')
    print("SCHEME TABLE")
    import struct
    for i in range(128):
        ptr=struct.unpack("<Q",pe.get_data(0x1f1c0+i*8,8))[0]
        if not ptr: break
        if not base <= ptr < base+pe.OPTIONAL_HEADER.SizeOfImage: break
        name=struct.unpack("<Q",pe.get_data(ptr-base,8))[0]
        print(i,hex(ptr-base),pe.get_string_at_rva(name-base),pe.get_data(ptr-base+0x78,12).hex())
    print("PUBLIC EXPORT RVAS")
    for e in pe.DIRECTORY_ENTRY_EXPORT.symbols:
        if e.name and e.name.startswith(b"curl_"):
            print(f"{e.name.decode()} {e.address:05X}")
if __name__ == "__main__":
    main()
