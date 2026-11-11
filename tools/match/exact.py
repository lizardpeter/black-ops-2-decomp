"""Find unique exact x86 function-body matches between two Ghidra exports."""
import argparse
import csv
from collections import defaultdict
from pathlib import Path


def read_tsv(path):
    with path.open(newline="") as stream:
        return list(csv.DictReader(stream, delimiter="\t", quoting=csv.QUOTE_NONE))


def by_body(path):
    functions = defaultdict(list)
    for row in read_tsv(path):
        key = (int(row["size"]), row["bytes_sha256"])
        functions[key].append(row)
    return functions


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path, help="reference ExportAll functions.tsv")
    parser.add_argument("retail", type=Path, help="retail ExportAll functions.tsv")
    parser.add_argument("linker_map", type=Path, help="reference MAP publics.tsv")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    reference = by_body(args.reference)
    retail = by_body(args.retail)
    symbols = defaultdict(list)
    for row in read_tsv(args.linker_map):
        if "f" in row["flags"]:
            symbols[row["va"]].append(row)

    candidates = []
    for (size, digest), source in reference.items():
        target = retail.get((size, digest), [])
        if size < 24 or len(source) != 1 or len(target) != 1:
            continue
        ref, dst = source[0], target[0]
        if ref["thunk"] or dst["thunk"] or int(ref["instructions"]) < 8:
            continue
        if ref["instructions"] != dst["instructions"]:
            continue
        for symbol in symbols[ref["va"]]:
            candidates.append((dst["va"], ref["va"], symbol["name"], symbol["lib"],
                               symbol["obj"], size, ref["instructions"], digest))

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(("retail_va", "reference_va", "symbol", "lib", "obj", "size",
                         "instructions", "bytes_sha256"))
        writer.writerows(sorted(candidates))
    print(f"{len({row[0] for row in candidates}):,} unique retail body matches; "
          f"{len(candidates):,} reference symbols")


if __name__ == "__main__":
    main()
