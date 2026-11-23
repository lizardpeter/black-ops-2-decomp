#!/usr/bin/env python3
"""Report static call reachability in Ghidra-exported BO2 original DLLs.

Inputs: functions.tsv and callgraph.tsv from the DLL Ghidra Actions artifacts.
No code execution. Indirect calls, callbacks, dynamically loaded imports,
and undecoded thunks are NOT necessarily included.
"""
import argparse
import csv
import json
from collections import defaultdict, deque
from pathlib import Path


def summarize(folder: Path, roots: list[str]) -> dict:
    with (folder / "functions.tsv").open(newline="") as f:
        functions = {int(r["va"], 16): r for r in csv.DictReader(f, delimiter="\t")}
    outgoing: dict[int, set[int]] = defaultdict(set)
    with (folder / "callgraph.tsv").open(newline="") as f:
        for r in csv.DictReader(f, delimiter="\t"):
            outgoing[int(r["caller"], 16)].add(int(r["callee"], 16))
    results = []
    for root in roots:
        matches = [va for va, r in functions.items() if r["name"] == root]
        if not matches:
            raise ValueError(f"{root!r} missing from {folder}/functions.tsv")
        for va in matches:
            distances = {va: 0}
            queue = deque([va])
            non_functions: set[int] = set()
            while queue:
                src = queue.popleft()
                for dst in outgoing.get(src, ()):
                    if dst not in functions:
                        non_functions.add(dst)
                    elif dst not in distances:
                        distances[dst] = distances[src] + 1
                        queue.append(dst)
            results.append({
                "root": root,
                "rva_or_va": f"{va:016X}",
                "reachable_static_functions": len(distances),
                "depths": {str(d): sum(x == d for x in distances.values())
                           for d in sorted(set(distances.values()))},
                "direct_callees": [
                    {"va": f"{dst:016X}", "name": functions[dst]["name"] if dst in functions else None}
                    for dst in sorted(outgoing.get(va, ()))
                ],
                "non_function_targets": [f"{dst:016X}" for dst in sorted(non_functions)],
            })
    return {"ghidra_function_inventory_count": len(functions),
            "distinct_direct_edges": sum(map(len, outgoing.values())),
            "note": "Conservative Ghidra static graph, NOT complete transitive runtime reachability.",
            "roots": results}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("analysis_folder", type=Path)
    ap.add_argument("function_names", nargs="+")
    ap.add_argument("--json", type=Path)
    args = ap.parse_args()
    result = summarize(args.analysis_folder, args.function_names)
    for root in result["roots"]:
        print(f"{root['root']}: {root['reachable_static_functions']} static functions, "
              f"{len(root['direct_callees'])} direct targets, "
              f"{len(root['non_function_targets'])} unmatched/external call targets")
        for callee in root["direct_callees"]:
            print(f"  {callee['va']} {callee['name'] or 'unmapped/external'}")
    print(result["note"])
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(result, indent=2) + "\n")


if __name__ == "__main__":
    main()
