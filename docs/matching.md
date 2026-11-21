# Matching builds without symbols to the references

Two tools give functions in a build without symbols (the Store target, the Steam exe) the identity of reference
functions. Neither uses address position: the Store build is a different architecture, linked with LTCG.

## tools/match/match.py: content and call graph

`python tools/match/match.py <reference> <target> [--originals DIR] [--same-compiler]` writes
`analysis/<target>/match/functions.tsv` (generated) with method, confidence and evidence per pair. Inputs:
`analysis/<build>/functions.tsv` and `ghidra/callgraph.tsv` for both builds (the Ghidra workflow), and both original
exes (`tools/fetch_originals.py`, into `originals/` by default).

Stages, as in Ghosts:

1. **string**: a string referenced by exactly one function in each build.
2. **constant**: at least two distinctive immediates, each unique to one function on both sides.
3. **named**: a string tied to exactly one function by a registration (`Cmd_AddCommand("name", Func)`) or a
   `{name, ..., function}` table entry.
4. **callee / caller**: call-graph propagation from matched pairs, with structural checks.

The scan reads the PE machine type. x64 code reaches data RIP-relatively (`lea rcx, [rip+disp]`); x86 code by
absolute address, as an immediate (`push offset aName`) or a base-less memory operand. Both resolve to the same
strings and functions, so the x86 servers match the x64 Store build directly. Tables use the build's pointer size.

### Validation

`CoDMPServer_s_PC` (the PC server's ship configuration) is analysed blind (`ghidra_pdb = false`), matched against
`CoDMPServer_PC`, and scored with its own PDB/MAP:

    python tools/match/match.py CoDMPServer_PC CoDMPServer_s_PC --same-compiler
    python tools/match/validate.py CoDMPServer_PC CoDMPServer_s_PC

On the blind analysis published at 69fed58: **6,933 of 18,659 ship-build functions matched (37.2%)** in ten
propagation rounds, **98.3% precision** (6,757 correct, 118 wrong, 58 unverifiable):

| method | correct | wrong | unverifiable | precision |
|---|---:|---:|---:|---:|
| string | 2,631 | 45 | 2 | 98.3% |
| named | 115 | 2 | 0 | 98.3% |
| constant | 36 | 0 | 1 | 100.0% |
| callee | 2,175 | 41 | 28 | 98.1% |
| caller | 1,800 | 30 | 27 | 98.4% |

That pair shares compiler and architecture, so it measures the method, not the harder x86-to-x64 case: Store matches
are reviewed before they become records.

### CoDMPServer_PC → t6mp_msstore (x86 → x64)

First run, on the Ghidra analyses published at 5d2be62: **2,973 of 18,495 target functions matched (16.1%)** in six
propagation rounds. By method: string 1,566, named 440, callee 498, caller 461, constant 8. A random sample of each
method reviewed by hand was consistent: unique error strings (`GScr_SetEMPJammed`, `Demo_ReadFooter`), registrations
(`UI_ChangeMenuOpenSlideDirection_f` under `changeMenuOpenSlideDirection`), OpenSSL's SHA-512 block function by its
round constants. Few constant matches is expected: x64 code builds many values differently and the 2024 build
changed data layouts.

These are generated identifications, not records: they become `records/names.tsv` rows (and `named` in STATUS.md)
once reviewed, starting with the subsystems being reconstructed.

## tools/match/behaviour.py: small leaves by behaviour

Short leaf functions often have no string, constant or call edge to anchor them. `python tools/match/behaviour.py <build> [name ...]` takes the reference server's original function as the oracle
(run on its differential case's inputs) and runs every candidate entry of the target (x64 `.pdata` starts, direct call targets, and 16-byte-aligned entries after `int3` padding: 37,499 in
`t6mp_msstore`) under Unicorn on probe inputs, and runs the full input set on survivors. Return values and buffers are compared, not
global writes (the builds' globals are at different addresses). Output: `analysis/<build>/match/behaviour.tsv`.

Results on `t6mp_msstore`:

| function | result |
|---|---|
| `LongSwap` | unique: `1407B51D0` (recorded, `matches`) |
| `ShortNoSwap`, `Long64NoSwap` | ambiguous: identity functions, folded (ICF) with every other body that returns its argument |
| `ClampChar`, `ClampShort`, `Com_HashKey`, `ShortSwap`, `Long64Swap`, `ColorIndex`, `I_islower`, `I_isdigit` | no standalone body: inlined |

Behaviour establishes equivalence, not original identity, so such names are recorded with `medium` confidence unless
other evidence (a caller matched by match.py) confirms them.
