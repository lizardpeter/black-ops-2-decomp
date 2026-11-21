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

On the blind analysis published at 69fed58, with the inlining-aware string votes, named globals, call order and
hub votes (below): **7,808 of 18,659 ship-build functions matched (41.8%)** in nine propagation rounds, **98.2%
precision** (7,607 correct, 137 wrong, 64 unverifiable; before those stages, 6,933 at 98.3%):

| method | correct | wrong | unverifiable | precision |
|---|---:|---:|---:|---:|
| string | 2,662 | 48 | 2 | 98.2% |
| named | 132 | 2 | 0 | 98.5% |
| global | 247 | 5 | 0 | 98.0% |
| constant | 40 | 0 | 1 | 100.0% |
| callee | 1,927 | 43 | 19 | 97.8% |
| caller | 1,915 | 32 | 20 | 98.4% |
| order | 630 | 5 | 22 | 99.2% |
| votes | 54 | 2 | 0 | 96.4% |

Stages added for the Store build's link-time code generation:
- **Inlined strings.** One target function holding the unique strings of several reference functions is the one
  among them that calls the others (within three calls): it inlined them.
- **Named globals.** A global the return value of a call is stored into, with the call's one identifier-like string
  (`g = Dvar_RegisterBool("name", ..., "description")`), is the same variable in both builds. A function that is the
  only one referencing such a global, or the only one with a given set of two or more, is the same function.
- **Call order.** Within a matched pair, the calls between two calls to matched functions are the same calls in the
  same order, except what the target inlined: one unmatched callee on each side in a short gap pairs up when the two
  look alike (size, callee count; an inlined reference callee leaves its own callee in its place).
- **Hub votes.** Functions called from everywhere (never alone in a gap) pair when gaps of equal length across at
  least three callers pair them the same way, and most of each side's votes agree.

That pair shares compiler and architecture, so it measures the method, not the harder x86-to-x64 case: Store matches
are reviewed before they become records.

### CoDMPServer_PC → t6mp_msstore (x86 → x64)

First run, on the Ghidra analyses published at 5d2be62: 2,973 of 18,495 target functions (16.1%). With the stages
above: **3,391 (18.3%)**: string 1,647, named 453, callee 488, caller 486, order 151, global 129, votes 29, constant
8. The Store build is harder than the ship build: most small functions are inlined, and it is a later source
revision without the servers' development output (`CL_MouseMove`'s `cl_showMouseRate` print is gone, so the
`Com_Printf` calls that anchor many server functions aren't there). Hand-identified Store functions go to
`records/names.tsv` with their evidence (structure, behaviour).

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
| `I_strnicmp`, `I_strncpyz`, `I_atoui`, `I_CleanStr`, `I_itoa`, `I_DrawStrlen`, `Com_IsLegacyXModelName` | unique (recorded, `matches`): `140471B50`, `140471B00`, `140471A90`, `140471E50`, `140471F70`, `1403E9910`, `1404732C0` |
| `Info_RemoveKey` | unique, but wrong: `1404723A0` is `Info_RemoveKey_Big` (its oversize message and 8 KB key buffer say so), which behaves the same on every input short enough for both. Recorded under that name |
| `ShortNoSwap`, `Long64NoSwap` | ambiguous: identity functions, folded (ICF) with every other body that returns its argument |
| `ClampChar`, `ClampShort`, `Com_HashKey`, `ShortSwap`, `Long64Swap`, `ColorIndex`, `I_islower`, `I_isupper`, `I_isdigit`, `I_CleanChat`, `I_strncmp`, `I_stristr`, `I_stricmpwild`, `Info_Validate`, `OrientationConcatenate`, `OrientationInvert`, `CMD_GetAnalogButtonValue`, `Com_GetFilenameSubString`, `Com_GetExtensionSubString`, `Com_StripExtension` | none: inlined, or not leaves the search can run (`tolower` and `strchr` are imports, which the harness stubs) |

Behaviour establishes equivalence, not original identity, so such names are recorded with `medium` confidence unless
other evidence (a caller matched by match.py) confirms them. Sibling functions are the trap: a match only says the
target agrees on the inputs the case has, so a case should include inputs that tell a function from its siblings
(`Info_RemoveKey` and `_Big` differ only past 1,023 characters, where the oracle needs a faked `Com_Error`, which
behaviour.py doesn't install).
