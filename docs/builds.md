# Build provenance

| Build | Role | PE timestamp (UTC) | CodeView record |
|---|---|---|---|
| `t6mp` | Retail PC multiplayer target | 2015-01-30 01:23:23 | `t6mp.pdb`, `DE575CD6-25F4-4C45-81EB-60450E0D2A61`, age 1 |
| `CoDMPServer_PC` | PC dedicated server reference | 2013-03-11 19:15:45 | `CoDMPServer_PC.pdb`, `3C12954B-A78E-4C6E-B4E8-5A8FD595F0DB`, age 51 |

Both files were downloaded from R2, hashed, and inspected as PE32 with a preferred image base of `00400000` and linker version 10.00. The downloaded server PDB's GUID and age agree with the executable. The retail executable is the 2015 object in R2. Its hash and CodeView identity identify this specific build.

The first server pass uses linker MAP publics. Reconstructed server routines are compared with the pinned x86 instructions through the differential harness. The three bootstrap routines have no verbatim byte match in the retail executable. Retail identities remain open. The retail inventory records PE imports and sections. Retail Ghidra output requires review before claiming function boundaries or names for that build.
