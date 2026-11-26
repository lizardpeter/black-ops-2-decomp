# Store MP renderer dvar registration: 14070E230

The previously unidentified 14070E230..1407175D7 routine (0x93a8 bytes inclusive) is the large renderer registration body, provisionally associated with gfx_d3d/r_dvars.cpp. It starts with renderer initialization call 140763730, then r_fontResolution, r_resolution, r_ssao and related settings. The Ghidra pseudocode is a timeout marker, not recovered source.

The companion TSV records all 665 direct registration/lookup call instructions, with 663 name-bearing sites and 359 distinct resolved literal names. 661 are direct string LEA matches; two more are recovered from the branch predecessor. These are static call instructions, not 665 distinct registrations. The lookup plus both branches count separately, and loop execution can add dynamic names.

| Kind | Sites |
| --- | ---: |
| find | 148 |
| reregister | 147 |
| new | 147 |
| bool | 28 |
| float | 121 |
| enum | 13 |
| color | 5 |
| vec3 | 17 |
| vec4 | 39 |

Unresolved name operands:
- 14070FAAE: indexed pointer table at 1409AD3F0, loop count depends on an existing dvar and hardware-related global 14406FC3C. The name is intentionally not assigned to one string.
- 140712CBB: 14093C480 points one byte before the exported string fragment c_LIB. Do not promote the likely vc_LIB spelling without original bytes.

Branch-carried names: 140710593 uses 1409AC200 from 14071034E (conditional branch at 140710373); 140712822 uses r_enableFlashlight at 1409AC7E0 from 140712796 (branch 1407127B6).

The source evidence includes the full 408,388-character disassembly section, 0x2bb0-byte stack allocation, conditional renderer mode paths, indexed registration loop and global result stores. Reconstruct the entire body in its source file rather than flattening into a list; recover the original PE float constants, enum tables, conditional flags and setter/subroutine boundaries first. This TSV is an evidence inventory only; no renderer runtime changes or body claim.
