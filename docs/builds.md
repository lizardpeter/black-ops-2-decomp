# Build provenance

The target is the Microsoft Store build (docs/store-build.md). This page covers the reference and related builds.

| Build | Role | PE timestamp (UTC) | CodeView record |
|---|---|---|---|
| `t6mp_msstore` | Store multiplayer, x64: **final target** | 2024-12-10 22:23:11 | `t6mp_msstore.pdb`, `ABBFB2F5-97CF-4E1D-8D20-67833D167727`, age 3 |
| `CoDMPServer_PC` | PC dedicated server, Release: primary reference (PDB + MAP) | 2013-03-11 23:15:45 | `c:\t6\game\pc\CoDMPServer_PC.pdb`, `3C12954B-A78E-4C6E-B4E8-5A8FD595F0DB`, age 51 |
| `CoDMPServer_s_PC` | PC dedicated server, ReleaseShip (PDB + MAP): matcher ground truth, analysed blind | 2013-03-11 23:21:34 | `c:\t6\game\pc\CoDMPServer_s_PC.pdb`, `A9CA5DF4-97BA-42DE-A071-C8F69DBA6EC8`, age 52 |
| `CoDMPServer_XBOX`, `_s_XBOX` | Xbox 360 host server, Release / ship (PDB) | 2013-03-13 01:49:15 / 02:04:03 | `D:\t6\tu8\game\pc\CoDMPServer.pdb` / `CoDMPServer_s.pdb` |
| `CoDMPServer_PS3`, `_s_PS3` | PS3 host server, Release / ship (PDB) | 2013-03-05 21:35:28 / 21:31:26 | `D:\t6\game\pc\CoDMPServer.pdb` / `CoDMPServer_s.pdb` |
| `CoDMPServer_WIIU`, `_s_WIIU` | Wii U host server, Release / ship (PDB) | 2013-01-22 02:51:38 / 21:28:43 | `c:\t6\game\pc\CoDMPServer.pdb` / `CoDMPServer_s.pdb` |
| `t6mp` | Steam multiplayer (2015), related | 2015-01-30 01:23:23 | `t6mp.pdb`, `DE575CD6-25F4-4C45-81EB-60450E0D2A61`, age 1 |

All servers are Windows x86 exes from the "Server Files [2013-03-11]" drop in R2 (`bo2/exes/`), each with its full
private PDB; only the PC pair also has linker MAPs. The console-host servers ran matches for each console and differ in
platform code; the Xbox 360 host is the newest (title update 8 tree, `D:\t6\tu8`). The PC pair's hashes and identities
agree with `lizardpeter/bo2-pc-server-decompile`'s `config/reference_builds.json`.

## Steam t6mp (related build)

The Steam exe and the PC server were downloaded from R2, hashed, and inspected as PE32 with a preferred image base of `00400000` and linker version 10.00. The downloaded server PDB's GUID and age agree with the executable. The retail executable is the 2015 object in R2. Its hash and CodeView identity identify this specific build.

The server linker MAP supplies symbol names and object files. The retail PE inventory records imports and sections.

| Routine | Server VA | Retail VA |
|---|---|---|
| `ClampChar` | `00761340` | `007043C0` |
| `ClampShort` | `00761360` | `00645340` |
| `Com_HashKey` | `0076B690` | `006F5A40` |
| `ColorIndex` | `0078DB60` | `004017C0` |
| `I_islower` | `0078DF00` | `00444010` |
| `I_isdigit` | `0078DF40` | `006580F0` |

The retail routines use stack-pointer-relative arguments in place of the server's frame-pointer-relative arguments. The matching evidence is the control flow and constants in each body, followed by differential execution of the pinned retail bytes. Both library variants run the same checks against both original builds.

## Retail function entry inventory

`analysis/t6mp/direct-entries.tsv` inventories decoded direct calls within the retail `.text` section to 16-byte-aligned instruction boundaries immediately after `INT3` padding. Each row includes the number of decoded call sites. The scan can miss indirectly reached functions and entries with other alignment or padding. It supplies candidate starts for comparison with the Ghidra inventory.

Regenerate the table with GNU `objdump` and the installed differential test requirements.

```sh
python tools/inventory/retail_calls.py originals/t6mp.exe
python tools/status.py
```

`analysis/t6mp/inventory.tsv` records the Ghidra 12.1.4 function entry addresses and body sizes for the pinned retail executable. The analysis used two CPUs, a 6,200 MiB Java heap, and disabled the x86 Constant Reference Analyzer. The saved project supports independent export and decompilation passes.

Ghidra recovered 24,565 functions. Of those, 18,593 overlap the direct-call scan and 5,972 are additional entries. The 735 direct-call candidates outside the Ghidra inventory remain available for review. All six differentially verified retail routines appear in both tables.

## Retail pseudocode

The Ghidra export in [`analysis/t6mp/pseudocode/`](../analysis/t6mp/pseudocode/) uses the same layout as Ghosts and BO3. Functions without a source-file assignment are grouped by 64 KiB address block under `_unattributed/`. The [function table](../analysis/t6mp/functions.tsv) records the decompiler outcome for every inventory entry.

| Outcome | Functions |
|---|---:|
| Generated C pseudocode | 24,392 |
| Thunks recorded with their targets | 168 |
| Decompiler errors | 5 |

The five failures are marked at their addresses in the generated files. Export validation found exactly one decompiler result for every non-thunk function. These files contain generated analysis. Verified reconstructions are tracked independently in [STATUS.md](../STATUS.md).
