"""Inventory probable t6mp function entries from direct calls and x86 padding."""
import argparse
from collections import Counter
import hashlib
from pathlib import Path
import re
import subprocess
import tomllib

import pefile

ROOT = Path(__file__).resolve().parents[2]
INSTRUCTION = re.compile(r"^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2}\s+)+\s*([a-z][a-z0-9.]*)\s*(.*)$")
DIRECT_TARGET = re.compile(r"^(?:0x)?([0-9a-f]+)(?:\s|$)")


def direct_entries(executable, text_start, code):
    text_end = text_start + len(code)
    candidates = {
        text_start + offset
        for offset in range(16, len(code), 16)
        if code[offset - 1] == 0xcc and code[offset] not in (0x00, 0x90, 0xcc)
    }
    starts = set()
    calls = Counter()
    command = ["objdump", "-d", "-j", ".text", "--insn-width=16", str(executable)]
    with subprocess.Popen(command, stdout=subprocess.PIPE, text=True) as disassembler:
        for line in disassembler.stdout:
            match = INSTRUCTION.match(line)
            if not match:
                continue
            address, mnemonic, operands = match.groups()
            address = int(address, 16)
            if address in candidates:
                starts.add(address)
            if mnemonic not in {"call", "calll"} or not text_start <= address < text_end:
                continue
            if target := DIRECT_TARGET.match(operands):
                destination = int(target[1], 16)
                if destination in candidates:
                    calls[destination] += 1
        if disassembler.wait():
            raise subprocess.CalledProcessError(disassembler.returncode, command)
    return [(address, calls[address]) for address in sorted(starts & calls.keys())]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    args = parser.parse_args()
    config = tomllib.loads((ROOT / "target.toml").read_text())
    build = next(build for build in config["build"] if build["id"] == "t6mp")
    binary = args.executable.read_bytes()
    if hashlib.sha256(binary).hexdigest() != build["exe"]["sha256"]:
        parser.error("Original t6mp SHA-256 mismatch")
    pe = pefile.PE(data=binary, fast_load=True)
    section = next(s for s in pe.sections if s.Name.rstrip(b"\0") == b".text")
    code = section.get_data(ignore_padding=True)
    entries = direct_entries(args.executable, pe.OPTIONAL_HEADER.ImageBase + section.VirtualAddress, code)
    if not entries:
        parser.error("no direct-call entries found in .text")

    output = ROOT / "analysis" / build["id"] / "direct-entries.tsv"
    with output.open("w", newline="\n") as stream:
        stream.write("va\tdirect_calls\n")
        for address, count in entries:
            stream.write(f"{address:08X}\t{count}\n")
    print(f"{len(entries):,} aligned, padded retail entries with direct calls")


if __name__ == "__main__":
    main()
