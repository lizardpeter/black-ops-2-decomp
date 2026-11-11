# Build provenance

| Build | Role | PE timestamp (UTC) | CodeView record |
|---|---|---|---|
| `t6mp` | Retail PC multiplayer target | 2015-01-30 01:23:23 | `t6mp.pdb`, `DE575CD6-25F4-4C45-81EB-60450E0D2A61`, age 1 |
| `CoDMPServer_PC` | PC dedicated server reference | 2013-03-11 19:15:45 | `CoDMPServer_PC.pdb`, `3C12954B-A78E-4C6E-B4E8-5A8FD595F0DB`, age 51 |

Both files were downloaded from R2, hashed, and inspected as PE32 with a preferred image base of `00400000` and linker version 10.00. The downloaded server PDB's GUID and age agree with the executable. The retail executable is the 2015 object in R2. Its hash and CodeView identity identify this specific build.

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
