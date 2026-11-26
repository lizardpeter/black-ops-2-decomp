# Engine-wide Store dvar registration inventory

Run:

    python tools/inventory/store_dvar_registration.py --summary --sites /tmp/store-dvar-sites.tsv > /tmp/store-dvar-owners.tsv

This static inventory uses the full MP Store disassembly. It separates core wrappers, lookup-only functions and registration candidates. The address of every call or tail jump is retained in the optional site TSV. It is a worklist, not a runtime trace; paired new/reregister branches count twice and the wrappers themselves call the core. Indirect calls and additional inlined paths without these boundaries are not covered. ZM/SP need independent address maps and disassembly.

Manual cross-check of the nine MP disassembly shards found 186 owning functions and 4,814 direct call/jump sites (including lookup-only and core wrapper internals). The totals by target were find 996, new 888, reregister 888, bool 564, int 8, float 1095, vec2 27, vec3 36, vec4 40, string 173, enum 40, color 59. These are not 4,814 unique dvars.

Highest-priority enclosing functions, with subsystem hypotheses based on literal strings and existing identifications:

| MP address | Direct sites | Evidence / likely owner |
| --- | ---: | --- |
| 14070E230 | 665 | Ghidra pseudocode timed out; recover from disassembly and rerun decompiler with larger timeout |
| 1401D1D10 | 348 | QoS/party settings, party migration and connection |
| 14061BAF0 | 334 | Shared gameplay settings; bg_viewKickScale, charge shot settings |
| 140298510 | 307 | Game settings, g_gametype and other game dvars |
| 140101580 | 230 | CG_RegisterDvars_Project; complete candidate already authored |
| 1403F9B20 | 187 | UI_RegisterDvars; prior complete candidate |
| 14039E2B0 | 178 | Common engine registration; com_maxfps and LUI settings |
| 140487D00 | 123 | WebM and livestream settings |
| 1403D4520 | 120 | Server registration; sv_clientFpsLimit, hostname, protocol |
| 1405ACD40 | 120 | Stats and publisher storage settings |
| 1405D3D20 | 119 | Demo recording/playback settings |
| 1405A2020 | 90 | Player stats, skill and related settings |

Do not promote the provisional subsystem labels to original symbol names without corroborating PDB/reference evidence. First reconstruct common/core, then client and renderer initialization in their original translation-unit locations. Keep limits and defaults faithful until enhanced overrides can be independently tested. No changes to active FOV/FPS settings are part of this inventory.
