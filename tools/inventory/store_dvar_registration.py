#!/usr/bin/env python3
"""Inventory Store registration call sites from complete Ghidra disassembly shards.

Static candidates, not runtime registration counts. Includes direct tail jumps,
both inline registration branches, core wrapper internals and lookup-only
callers. The MP map is pinned to the Store build in target.toml. Do not apply
its addresses to ZM or SP. Future maps must be independently established.
"""
from collections import Counter, defaultdict
import argparse
import csv
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
TARGETS = {
    "14046B8B0": "find", "14046C1A0": "new", "14046BF10": "reregister",
    "14046C4C0": "bool", "14046C580": "int", "14046C650": "float",
    "14046C760": "vec2", "14046C840": "vec3", "14046C930": "vec4",
    "14046CA30": "string", "14046CAF0": "enum", "14046CBD0": "color",
}
HEADER = re.compile(r"^; ([0-9A-Fa-f]+)\s")
TRANSFER = re.compile(r"\b(CALL|JMP) 0x([0-9a-f]+)\s*$", re.I)
INSTRUCTION = re.compile(r"^\s*([0-9A-Fa-f]+)\s+[0-9A-Fa-f]+\s+")
CORE = set(TARGETS)
ORDER = tuple(TARGETS.values())

def scan(files):
    owners = defaultdict(Counter)
    sites = []
    for path in sorted(files):
        owner = None
        with path.open(encoding="utf-8") as src:
            for line in src:
                header = HEADER.match(line)
                if header:
                    owner = header[1].upper()
                    continue
                match = TRANSFER.search(line)
                insn = INSTRUCTION.match(line)
                if not owner or not match or not insn:
                    continue
                target = match[2].upper()
                if target not in TARGETS:
                    continue
                kind = TARGETS[target]
                owners[owner][kind] += 1
                sites.append((owner, insn[1].upper(), match[1].upper(), target, kind))
    return owners, sites

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--disasm", type=Path, default=ROOT/"analysis/t6mp_msstore/disasm/_unattributed")
    parser.add_argument("--sites", type=Path, help="Optional per-instruction TSV output")
    parser.add_argument("--summary", action="store_true", help="Print aggregate counts to stderr")
    args = parser.parse_args()
    files = sorted(args.disasm.glob("*.asm"))
    if len(files) != 9:
        parser.error("expected all nine MP Store disassembly shards")
    owners, sites = scan(files)
    if not owners:
        parser.error("no mapped MP call sites; check disassembly/build")
    out = csv.writer(sys.stdout, delimiter="\t", lineterminator="\n")
    out.writerow(("owner", "classification", "raw_sites", *ORDER))
    for owner, counts in sorted(owners.items(), key=lambda x: (-sum(x[1].values()), x[0])):
        role = ("core" if owner in CORE else
                "lookup_only" if sum(counts.values()) == counts["find"] else
                "registration_candidate")
        out.writerow((owner, role, sum(counts.values()), *(counts[k] for k in ORDER)))
    if args.sites:
        with args.sites.open("w", encoding="utf-8", newline="") as f:
            writer = csv.writer(f, delimiter="\t", lineterminator="\n")
            writer.writerow(("owner", "instruction", "transfer", "target", "kind"))
            writer.writerows(sites)
    if args.summary:
        counts = Counter(k for _, _, _, _, k in sites)
        print(f"{len(files)} shards, {len(owners)} owning functions, {len(sites)} direct sites",
              file=sys.stderr)
        print(", ".join(f"{k}={counts[k]}" for k in ORDER), file=sys.stderr)

if __name__ == "__main__":
    main()
