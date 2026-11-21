# Working in black-ops-2-decomp

Decompilation of Call of Duty: Black Ops II (T6). The final target is the **Microsoft Store build** (x64, `kind =
"retail"` in `target.toml`), multiplayer (`t6mp_msstore`) first. The PC and console-host dedicated servers (x86, full
PDBs) are the references; the 2015 Steam `t6mp.exe` (x86) is a `related` helper. The layout and method follow
`lizardpeter/ghosts-decomp` and `lizardpeter/black-ops-3-decomp`; this repo owns its copy of the tools.

## The one rule: generated vs authored

| Kind | Where | Who writes it |
|---|---|---|
| **Generated** | `analysis/<build>/`, `STATUS.md` | Only the scripts in `tools/` (and the Ghidra workflow). Never hand-edit. |
| **Authored** | `src/`, `include/`, `enhanced/`, `tests/`, `docs/`, `records/` | People and agents. |

If a generated file is wrong, fix the script that wrote it and re-run. Don't patch the output.

| Path | Responsibility |
|---|---|
| `target.toml` | Every original build: kind, architecture, R2 keys, SHA-256, PDB identity, `analyze` flag |
| `analysis/<build>/` | PE inventory (`pe.json`, x64 `pdata.tsv`), PDB/MAP tables (`pdb/`, `map/`), Ghidra exports (`ghidra/`, saved project split under 100 MB), `functions.tsv`, `pseudocode/`, `disasm/`, `match/`, `logs/` |
| `src/`, `include/` | Faithful behaviour, including original quirks |
| `enhanced/` | Replacement translation units for intentional improvements (`enhanced/sources.cmake`) |
| `records/` | `functions.tsv` (status), `names.tsv` (identifications in builds without symbols), `bugs.tsv`, `patches.tsv` |
| `tests/differential/` | Reconstructions against the original code under Unicorn, x86 or x64 |
| `tools/` | Fetching, inventory, PDB extraction, Ghidra scripts, matching, status and graph generation |

## Addresses

Upper-case hexadecimal without `0x`, zero-padded to at least eight digits: x86 builds at 0x00400000 read `0078DCF0`,
x64 builds at 0x140000000 read `1407B51D0`. Every tool uses that form. Server addresses apply to the server build only.

## Builds

- **Store (target):** `t6mp_msstore`, `t6zm_msstore`, `t6sp_msstore`. VS2019 16.11, LTCG, GDK, no Arxan, no Steam,
  no relocations. See `docs/store-build.md`.
- **References:** `CoDMPServer_PC` (PDB + MAP, primary) and its ship build `CoDMPServer_s_PC` (analysed blind: the
  matcher's ground truth), plus the Xbox 360, PS3 and Wii U host servers (PDBs). VS2010, x86. See `docs/builds.md`.
- **Related:** `t6mp`, the 2015 Steam exe, fully analysed.

## Function status ladder

`located → named → typed → decompiled → compiles → matches → integrated`

- `located`: entry and size known (`.pdata` gives every non-leaf x64 function; Ghidra the rest)
- `named`: original name known (PDB/MAP, or a recorded identification in `records/names.tsv`)
- `typed`: original prototype known
- `decompiled` / `compiles`: authored source exists / builds in CI
- `matches`: passes `tests/differential/reference.py` against that build's original bytes. Optimised builds won't
  byte-match; the test compares behaviour. CI fails if `records/functions.tsv` claims `matches` for a function the
  test does not cover, or the reverse.
- `integrated`: linked into the runnable reconstruction

`STATUS.md` and the `re_t6` graph import set are computed from `analysis/` and `records/` (`python tools/status.py`,
`python tools/graph/sync.py`). Don't track progress anywhere else.

## Identifications

Every cross-build identification in `records/names.tsv` records its method, evidence and confidence. Never match on
address position across differently linked builds. Methods: `string`, `constant`, `named`, `callee`, `caller`
(`tools/match/match.py`), `behaviour+differential` (`tools/match/behaviour.py`, for small leaves with no anchor),
`semantic+differential` (by reading). See `docs/matching.md`.

## Faithful and enhanced

`src/` is original behaviour, bugs included. The enhanced build replaces whole translation units through
`enhanced/sources.cmake`, each recorded in `records/patches.tsv`. Never fix a bug in `src/`; record it in
`records/bugs.tsv` (the enhanced build's fix queue: anything in the original worth fixing or improving, not only
outright bugs) and fix it in `enhanced/`.

## Originals

Game files are never committed. They live in R2 under the keys in `target.toml`, and every tool verifies the SHA-256
(`python tools/fetch_originals.py <build> originals`). Helpers for reading the bucket are not part of this repo.

## Ghidra pipeline

`.github/workflows/ghidra.yml` runs on push when `target.toml` or the pipeline changes, for builds with
`analyze = true` (or the builds named in a manual run). It extracts PDB/MAP tables (`tools/pdb/extract.py`), imports
each build (with its PDB unless `ghidra_pdb = false`), exports, decompiles in shards and commits `analysis/<build>/`.
Set `analyze` back to `false` once a build's analysis is committed.
