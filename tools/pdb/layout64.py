#!/usr/bin/env python3
"""Predict each PDB type's x64 layout from a 32-bit reference build's types.

    python tools/pdb/layout64.py [build]      (default CoDMPServer_PC)

Reads analysis/<build>/pdb/types.h (resymc's dump: every struct, union and class with its fields' x86 offsets) and
lays each type out again with MSVC's rules, twice: for x86, which must reproduce the PDB's offsets (the check on
the parser and on the inferred packing), and for x64, where pointers are 8 bytes. Writes
analysis/<build>/pdb/layout_x64/types.tsv and fields.tsv (generated). For a type whose x86 layout isn't reproduced
(a member the dump leaves out, a layout rule not modelled), fields.tsv lists only the fields before the first wrong
one.

The x64 layout is a prediction for the Store build: the same declarations compiled for x64. A revision that changed a
type (the Store's usercmd_s) shows up as a mismatch against the Store's code, which is what static_asserts check.

MSVC's rules as used here: a member is placed at the next multiple of min(its alignment, pack); a struct's alignment
is its largest member alignment (capped by pack, raised by __declspec(align)); its size is rounded up to it;
consecutive bit-fields of one type share a unit while they fit; a union is as large as its largest member; a class
with virtual functions and no base starts with a vfptr. Packing (#pragma pack) and __declspec(align) are not in the
PDB: each type gets the pack and alignment that reproduce its x86 offsets and size.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

PRIMS = {
    "char": 1, "signed char": 1, "unsigned char": 1, "bool": 1, "int8_t": 1, "uint8_t": 1, "__int8": 1,
    "int16_t": 2, "uint16_t": 2, "short": 2, "unsigned short": 2, "wchar_t": 2, "char16_t": 2,
    "int32_t": 4, "uint32_t": 4, "int": 4, "unsigned int": 4, "long": 4, "unsigned long": 4, "float": 4,
    "char32_t": 4, "HRESULT": 4,
    "int64_t": 8, "uint64_t": 8, "__int64": 8, "unsigned __int64": 8, "double": 8, "long long": 8,
    "unsigned long long": 8, "long double": 8,
    "__m64": 8, "__m128": 16, "__m128i": 16, "__m128d": 16, "__m256": 32,
}
HEAD = re.compile(r"^(struct|union|class) (.+?)(?: : (.+?))? \{ /\* Size=(0x[0-9a-f]+) \*/$")
FIELD = re.compile(r"^\s*/\* (0x[0-9a-f]+) \*/ (.+);(?: /\* BitPos=(\d+) \*/)?$")
ENUM = re.compile(r"^enum (\S+) : (\w+) \{")


class Agg:
    def __init__(self, kind, name, bases, size):
        self.kind, self.name, self.bases, self.size = kind, name, bases, size
        self.members = []  # Member or nested Agg (anonymous struct/union)
        self.virtual = False


class Member:
    def __init__(self, x86, decl, bitpos):
        self.x86, self.bitpos = x86, bitpos
        self.type, self.name, self.dims, self.bits = parse_decl(decl)


def parse_decl(decl):
    """'const char* name[4][2] : 3' -> (type text, name, [4, 2], bits or None)."""
    bits = None
    m = re.match(r"(.*\S)\s*:\s*(\d+)$", decl)
    if m and not decl.rstrip().endswith("::"):
        decl, bits = m.group(1), int(m.group(2))
    fp = re.match(r"(.+?)\(\s*\*\s*(\w+)\s*((?:\[\d+\])*)\)\s*\(.*\)$", decl)  # function pointer (or array of)
    if fp:
        return "void*", fp.group(2), [int(d) for d in re.findall(r"\[(\d+)\]", fp.group(3))], bits
    if bits is not None and re.fullmatch(r"[\w ]+", decl) and decl.strip() in PRIMS:
        return decl.strip(), "", [], bits  # an unnamed bit-field (": 0" closes the unit)
    m = re.match(r"(.*?)(\w+)\s*((?:\[\d+\])*)$", decl)
    if not m:
        return decl, decl, [], bits
    return m.group(1).strip(), m.group(2), [int(d) for d in re.findall(r"\[(\d+)\]", m.group(3))], bits


def parse(path):
    aggs, enums, fwd = {}, {}, set()
    stack = []
    for line in path.read_text(errors="replace").splitlines():
        e = ENUM.match(line)
        if e and not stack:
            enums[e.group(1)] = PRIMS.get(e.group(2), 4)
            continue
        h = HEAD.match(line)
        if h and not stack:
            bases = [b.strip().split()[-1] for b in h.group(3).split(",")] if h.group(3) else []
            stack = [Agg(h.group(1), h.group(2), bases, int(h.group(4), 16))]
            continue
        if not stack:
            continue
        s = line.strip()
        if s in ("union {", "struct {"):
            nested = Agg(s.split()[0], None, [], None)
            stack[-1].members.append(nested)
            stack.append(nested)
            continue
        if s == "};":
            done = stack.pop()
            fix_array_pointers(done)
            if not stack:
                aggs.setdefault(done.name, done)
            continue
        f = FIELD.match(line)
        if f:
            stack[-1].members.append(Member(int(f.group(1), 16), f.group(2), f.group(3)))
        elif s.startswith("virtual "):
            stack[0].virtual = True
    return aggs, enums


def fix_array_pointers(a):
    """resymc prints a pointer to an array (`T (*p)[16]`) as an array of pointers (`T* p[16]`): where the next
    member starts one x86 pointer later, it is the pointer (bgs_t.scriptmoveranim_trees)."""
    if a.kind == "union":
        return
    ms = [m for m in a.members if isinstance(m, Member)]
    for m, n in zip(ms, ms[1:]):
        if m.dims and "*" in m.type and n.x86 - m.x86 == 4:
            m.dims = []


class Layout:
    def __init__(self, aggs, enums, ptr):
        self.aggs, self.enums, self.ptr = aggs, enums, ptr
        self.cache = {}     # type name -> (size, align, {field path: offset}) or None
        self.policy = {}    # type name -> (pack, explicit align): set by the x86 fit, reused for x64
        self.need = {}      # type name -> alignment a containing type's PDB offsets require of it
        self.unknown = set()

    def explicit(self, t):
        """A type's __declspec(align): #pragma pack doesn't lower it."""
        t = " ".join(re.sub(r"\b(const|volatile|struct|union|class|enum)\b", "", t).split())
        return 1 if "*" in t or "&" in t else self.policy.get(t, (8, 1))[1]

    def type_size(self, t):
        """(size, align) of a member type, or None."""
        t = re.sub(r"\b(const|volatile|struct|union|class|enum)\b", "", t).strip()
        if "*" in t or "&" in t:
            return self.ptr, self.ptr
        t = " ".join(t.split())
        if t in PRIMS:
            return PRIMS[t], min(PRIMS[t], 16)
        if t in self.enums:
            return self.enums[t], self.enums[t]
        r = self.agg(t)
        if r is None:
            self.unknown.add(t)
            return None
        return r[0], r[1]

    def agg(self, name):
        if name in self.cache:
            return self.cache[name]
        a = self.aggs.get(name)
        if a is None:
            return None
        self.cache[name] = None  # a cycle reads as unknown
        pack, extra = self.policy.get(name, (8, 1))
        r = self.lay(a, pack, extra)
        self.cache[name] = r
        return r

    def lay(self, a, pack, extra):
        """(size, align, {path: offset}) of an aggregate, or None if a member's type is unknown."""
        fields, off, align = {}, 0, 1
        union = a.kind == "union"
        if a.bases:
            for b in a.bases:
                r = self.agg(b)
                if r is None:
                    return None
                al = max(min(r[1], pack), self.policy.get(b, (8, 1))[1])
                off = (off + al - 1) // al * al
                for k, v in r[2].items():
                    fields[f"{b}::{k}"] = off + v
                off += r[0] if r[0] > 1 or r[2] else 0  # an empty base takes no room
                align = max(align, al)
        elif a.virtual or (a.members and isinstance(a.members[0], Member) and a.members[0].x86 > 0
                           and a.kind == "class"):
            off, align = self.ptr, min(self.ptr, pack)
            fields["__vfptr"] = 0
        size = off
        unit = None  # (x86 offset, type, unit start) of the open bit-field unit
        for m in a.members:
            if isinstance(m, Agg):
                r = self.lay(m, pack, 1)
                if r is None:
                    return None
                msize, mal, sub = r
                mal = min(mal, pack)
                at = 0 if union else (off + mal - 1) // mal * mal
                for k, v in sub.items():
                    fields[k] = at + v
                off = at + msize if not union else off
                size = max(size, at + msize)
                align = max(align, mal)
                unit = None
                continue
            r = self.type_size(m.type)
            if r is None:
                return None
            esize, eal = r
            count = 1
            for d in m.dims:
                count *= d
            mal = max(min(eal, pack), self.explicit(m.type))
            if m.bits == 0:
                unit = None  # ": 0" closes the bit-field unit
                continue
            if m.bits is not None:
                if unit and unit[0] == m.x86 and unit[1] == m.type:
                    fields[m.name] = unit[2]
                    continue
                at = 0 if union else (off + mal - 1) // mal * mal
                unit = (m.x86, m.type, at)
                if m.name:
                    fields[m.name] = at
                off = at + esize if not union else off
                size = max(size, at + esize)
                align = max(align, mal)
                continue
            unit = None
            at = 0 if union else (off + mal - 1) // mal * mal
            fields[m.name] = at
            msize = esize * count
            off = at + msize if not union else off
            size = max(size, at + msize)
            align = max(align, mal)
        align = max(align, extra)
        size = (size + align - 1) // align * align if size else 1
        return size, align, fields


def x86_fields(a, prefix=""):
    """{path: PDB offset} for comparison (nested anonymous members flattened, as lay() names them)."""
    out = {}
    for m in a.members:
        if isinstance(m, Agg):
            out.update(x86_fields(m))
        elif m.name and m.name not in out and m.bits != 0:
            out[m.name] = m.x86
    return out


def member_types(a):
    """{member name: type text} of an aggregate, nested anonymous members included."""
    out = {}
    for m in a.members:
        if isinstance(m, Agg):
            out.update(member_types(m))
        elif m.name:
            out.setdefault(m.name, m.type)
    return out


def check(x86, aggs, name):
    """'ok', 'x86 mismatch' or 'unknown member type' for a type under its current policy, and the first wrong
    field: (field, PDB offset, computed offset)."""
    a = aggs[name]
    x86.cache.pop(name, None)
    r = x86.agg(name)
    if r is None:
        return "unknown member type", None
    want = x86_fields(a)
    for k, v in sorted(want.items(), key=lambda kv: kv[1]):
        if r[2].get(k) != v:
            return "x86 mismatch", (k, v, r[2].get(k))
    return ("ok", None) if r[0] == a.size else ("x86 mismatch", None)


def fit(x86, aggs, name):
    """Give a type the pack and size alignment that reproduce its PDB layout (keeping any alignment its containers
    showed it needs, x86.need). True if it fits."""
    a = aggs[name]
    floor = x86.need.get(name, 1)
    for pack in (8, 4, 2, 1):
        x86.policy[name] = (pack, floor)
        status, bad = check(x86, aggs, name)
        if status == "unknown member type":
            break
        if bad is None:  # fields right: an alignment that rounds the size up to the PDB's
            r = x86.agg(name)
            extra = floor
            while (r[0] + extra - 1) // extra * extra < a.size and extra < 4096:
                extra *= 2
            if (r[0] + extra - 1) // extra * extra == a.size:
                x86.policy[name] = (pack, extra)
                x86.cache.pop(name, None)
                return True
    x86.policy[name] = (8, floor)
    x86.cache.pop(name, None)
    return False


def raise_candidate(x86, aggs, name):
    """(member type, alignment) that would explain a type's first wrong field: a member placed further on than its
    type's alignment allows (__declspec(align(N)) on the member's type)."""
    status, bad = check(x86, aggs, name)
    if not bad or bad[2] is None or bad[1] <= bad[2]:
        return None
    t = " ".join(re.sub(r"\b(const|volatile|struct|union|class|enum)\b", "",
                        member_types(aggs[name]).get(bad[0], "")).split())
    if t not in aggs:
        return None
    # every power of two that moves the member exactly there (the smallest may only explain this one placement)
    return tuple((t, al) for al in (2 ** k for k in range(1, 13))
                 if (bad[2] + al - 1) // al * al == bad[1] and al > x86.need.get(t, 1)) or None


def fit_all(x86, aggs, names):
    x86.cache.clear()
    for name in names:
        fit(x86, aggs, name)
    x86.cache.clear()
    return {n for n in names if check(x86, aggs, n)[0] == "ok"}


def score(x86, aggs, names):
    """Fields placed right, up to each type's first wrong one: a raise that moves a mismatch further on is progress
    even while the type still has another one."""
    total = 0
    for n in names:
        r = x86.agg(n)
        if r is None:
            continue
        for k, v in sorted(x86_fields(aggs[n]).items(), key=lambda kv: kv[1]):
            if r[2].get(k) != v:
                break
            total += 1
    return total


def containers(aggs):
    """{type: types that hold it, directly or not}."""
    direct = {}
    for name, a in aggs.items():
        for t in set(member_types(a).values()) | set(a.bases):
            t = " ".join(re.sub(r"\b(const|volatile|struct|union|class|enum)\b", "", t).split())
            if t in aggs and "*" not in t:
                direct.setdefault(t, set()).add(name)
    out = {}
    for t in aggs:
        seen, todo = set(), [t]
        while todo:
            for c in direct.get(todo.pop(), ()):
                if c not in seen:
                    seen.add(c)
                    todo.append(c)
        out[t] = seen
    return out


def main():
    build = sys.argv[1] if len(sys.argv) > 1 else "CoDMPServer_PC"
    pdb = ROOT / "analysis" / build / "pdb"
    aggs, enums = parse(pdb / "types.h")
    x86 = Layout(aggs, enums, 4)
    # fit each type's packing and alignment to its PDB layout, dependencies first (agg() recurses)
    names = sorted(aggs)
    ok = fit_all(x86, aggs, names)
    # __declspec(align(N)) types: try each alignment a mismatch suggests, keep it only if more types then fit
    held = containers(aggs)
    tried = set()
    while True:
        cands = {c for n in names if n not in ok for c in (raise_candidate(x86, aggs, n) or ())} - tried
        if not cands:
            break
        by_type = {}
        for t, align in cands:
            by_type.setdefault(t, []).append(align)
        for t, aligns in sorted(by_type.items()):
            tried |= {(t, al) for al in aligns}
            scope = sorted({t} | held[t])
            old = x86.need.get(t)
            best, best_score = old, score(x86, aggs, scope)
            for al in sorted(aligns):
                x86.need[t] = al
                fit_all(x86, aggs, scope)
                sc = score(x86, aggs, scope)
                if sc > best_score:
                    best, best_score = al, sc
            if best is None:
                x86.need.pop(t, None)
            else:
                x86.need[t] = best
            ok = (ok - set(scope)) | fit_all(x86, aggs, scope)
        ok = fit_all(x86, aggs, names)  # a full pass: scoped fits leave the rest of the cache behind
    for _ in range(4):
        ok = fit_all(x86, aggs, names)
    status = {name: check(x86, aggs, name)[0] for name in names}
    x64 = Layout(aggs, enums, 8)
    x64.policy = x86.policy
    out = pdb / "layout_x64"
    out.mkdir(exist_ok=True)
    trows, frows = [], []
    for name in sorted(aggs):
        a = aggs[name]
        r = x64.agg(name)
        pack, extra = x86.policy[name]
        ok_type = status[name] == "ok"
        trows.append([name, a.kind, f"{a.size:X}", f"{r[0]:X}" if r and ok_type else "",
                      str(r[1]) if r and ok_type else "", str(pack), str(extra),
                      status[name] if r or not ok_type else "x64 unknown"])
        if not r:
            continue
        want = x86_fields(a)
        got86 = x86.agg(name)
        # a type that doesn't reproduce its x86 layout: only the fields before its first wrong one are predicted
        limit = None
        if not ok_type and got86:
            for k, v in sorted(want.items(), key=lambda kv: kv[1]):
                if got86[2].get(k) != v:
                    limit = v
                    break
        for k, v in sorted(r[2].items(), key=lambda kv: (kv[1], kv[0])):
            if limit is not None and (k not in want or want[k] >= limit):
                continue
            frows.append([name, k, f"{want[k]:X}" if k in want else "", f"{v:X}"])
    (out / "types.tsv").write_text("type\tkind\tx86_size\tx64_size\tx64_align\tpack\talign\tstatus\n"
                                   + "".join("\t".join(r) + "\n" for r in trows))
    (out / "fields.tsv").write_text("type\tfield\tx86_offset\tx64_offset\n"
                                    + "".join("\t".join(r) + "\n" for r in frows))
    ok = sum(1 for s in status.values() if s == "ok")
    print(f"{build}: {ok} of {len(aggs)} types reproduce their x86 layout; x64 layouts in {out.relative_to(ROOT)}")
    for s in ("x86 mismatch", "unknown member type"):
        print(f"  {s}: {sum(1 for v in status.values() if v == s)}")


if __name__ == "__main__":
    main()
