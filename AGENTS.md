# Working in black-ops-2-decomp

This repo follows the Ghosts and BO3 decompilation layout. T6 PC targets are PE32 x86. Write image virtual addresses as eight uppercase hexadecimal digits, including leading zeroes.

| Path | Responsibility |
|---|---|
| `target.toml` | Original build identities, R2 keys, SHA-256 hashes |
| `analysis/<build>/` | Tool-generated PE inventory, MAP/PDB tables, disassembly, and pseudocode |
| `src/`, `include/` | Faithful behavior, including original quirks |
| `enhanced/` | Replacement translation units for intentional improvements |
| `records/` | Authored reconstruction status, naming evidence, bug queue, and patches |
| `tests/differential/` | Automated comparisons with original x86 routines |
| `tools/` | Source retrieval, extraction, status and graph generation |

Regenerate analysis from pinned inputs when a tool needs correcting. Keep original binaries and symbols outside Git. `STATUS.md` and the `re_t6` graph import set are derived from the analysis and records. Run `python tools/status.py` and `python tools/graph/sync.py` to regenerate them.

Function status progresses through `located → named → typed → decompiled → compiles → matches → integrated`. Record `matches` only when an automated test compares the reconstructed routine with the pinned original. Record evidence and confidence for cross-build names. Server addresses apply to the server build.

Register an enhanced replacement in `enhanced/sources.cmake`, record its purpose and test in `records/patches.tsv`, and keep the original behavior in the faithful source. Both variants use the same CMake build.
