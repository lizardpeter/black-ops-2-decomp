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

## Original PE extraction and entry-prefix candidate

Run 45888511169 verified 37,800 bytes, 6,916 instructions, 496 block leaders, 672 calls, 665 exact inventory matches and 724 RIP-relative references. The original bytes at 14093C480 read `vc_LIB`. The first eight pointers at 1409AD3F0 resolve to r_smp_worker_thread0 through r_smp_worker_thread7; the loop bound remains unresolved. The output artifact contains annotated instructions and JSON with exact constant bits and branch/call boundaries.

`src/gfx_d3d/r_dvars_store.cpp` reconstructs only 14070E2B5..14070E516, including the initial subroutine, two inline integer registration branches and six SSAO settings. It is replay-only, not a complete renderer registration body. `tests/verify_store_renderer_prefix.py` cuts the original routine at 14070E517 and compares all four lookup branch combinations, ordered calls and seven global assignments. The new prefix test is pending CI; unused stack lanes are normalized and initialization is mocked.

## Extended entry reconstruction

The initial eight registrations passed differential replay (run 45888692737). The source now extends through 14070ECCF, including TXAA, clip encoding, window placement/fullscreen, convergence, hardware gamma and texture filtering. The three enum arguments are modeled as external original-table boundaries; the mock verifies their exact PE addresses, not their contents. The new test exhausts all 1024 lookup branch combinations; pending CI. No full-function or runtime claim.

## Worker registration fragment

14070F91F..14070FACD independently reconstructed. Hardware value at 14406FC3C is compared unsigned: <=2 => 1, <=4 => 2, <=10 => hardware-2, otherwise 8. The integer dvar r_smp_worker_threads has that default, domain 2..8, flags 0. The loop reads the current dvar with original type conversions and uses signed comparison, not the hardware-derived default; each worker gets default true and flags 0x4000 below the hardware threshold, otherwise false and 0x40. The original re-reads the global dvar pointer and hardware value each iteration. The source retains that behavior, assumes a valid registry count <=8, and is not connected to the entry prefix until the intervening code is recovered. Independent PE replay covers 1400 combinations, pending CI. No assumption about the precise producer of the hardware global.

## Contiguous texture, lighting and material registration extension

Entry source extended to 14070F91E, immediately before the independently recovered worker fragment. 83 contiguous registrations, 58 global assignments and 102 ordered events expected. Includes the conditional near-plane flags, color and vector registrations, and exact initial strings for 11 original enum tables. The penetration and reflection-probe table contents still require recovery; their original address arguments are checked. New prefix replay samples both mode-byte states and each lookup branch plus deterministic mixed patterns, rather than claiming exhaustive combinations. No live renderer integration. Pending CI.

## Post-worker continuation

14070FACE..140710BFF: 32 additional ordered registrations, including MSAA, vsync, clear colors, renderer limits, spot/flashlight shadows and visibility controls. The two flashlight branches are represented as one common sequence with the original mode-dependent flags. A recovered-prefix entry composes all three adjacent fragments (not yet tested as one continuous original execution). Isolated differential replay covers 17 lookup sites and two flashlight modes, pending CI; 25 result slots. No enhanced behavior changed.

## Verified entry and worker results; continuous replay

Run 45889760392 passed 2086 entry cases (102 events, 58 globals), 1400 isolated worker cases, and the enum evidence extractor. All 13 recovered renderer enum table strings are now defined in source; validation compares the original initial table entries and terminators. The re-registration probe now checks all meaningful vector default lanes, not only the first. The continuous replay runs the original instructions from 14070E230 through 140710BFF without a fragment-entry stub, comparing 153..161 events and 92 global slots in 512 mixed cases. This is pending CI; it still mocks the external functions and cuts before the remaining original body.

## Shadow and stereo continuation

Run 45889971187 passed 512 continuous replays of the prior prefix. New source extends through 1407126CF with 51 additional registrations, 33 inlined lookup paths and 32 new global assignments. r_forceLod uses a null-terminated five-entry enum table with default index 4. The new continuous test compares 237..245 events and 124 result slots across 512 cases. Pending CI; no native activation or full-function claim.

## Film and color-vector continuation

Source extends to 140712E97, with 21 further registrations and 15 global assignments, including the conditional player-shadow and flashlight branches, film controls, exp_softclip and nine vec4 controls. The continuous test now includes the original vec4 ABI with all four default lanes and both bounds. Pending CI; the full renderer body is still incomplete.

## Bloom and color-matrix continuation

Run 45890504379 passed continuous replay through 140712E97. New source extends through 14071376E with 22 additional registrations and 20 result slots, covering bloom, HDR depth of field, zombie darkness, color matrices and flare controls. The extended replay covers 293..301 events and 159 result slots. Pending CI. This is source reconstruction under a test guard, not runtime integration.

## Water and depth-of-field continuation

22 further registration operations through 140713DA0, including signed/unbounded script shader vector domains and depth of field. The continuous replay now expects 318..326 events and 181 global slots. Pending CI.

## Screen-effects continuation

Run 45891017028 passed 512 continuous replays (318..326 events, 181 globals) through the water/DOF section. New body extends through 14071478A, covering 30 additional flame, water-sheeting, anaglyph, rim, generic scene, revive and poison effect registrations and 23 result slots. The original three mode-dependent float setter calls are preserved with source 0 and their actual destination pointers; they are not folded into defaults. Replay now compares their order and argument values. Pending CI; not activated in the live game.

## Full body candidate

The screen-effects replay passed (run 45891360881). The final 98 registrations, 53 inlined lookup paths and 52 global assignments through 14071757E have now been authored. The final section includes the actual r_defaultMode and r_customMode empty string defaults, monitor and aspect ratio registration, both enum table scans, and the original type-dependent see-through decal setter with null-pointer/name checks. The complete replay executes the genuine original epilogue and RET instead of an artificial cut. The candidate compares 507..519 events and 256 global result slots; validation pending. It is not yet connected to the live registry. The earlier fragment tests remain in place.

## Full-body replay passed; opt-in native hybrid binding

Run 45891571445 passed 512 complete original-function replays, including the genuine RET, 507..519 ordered events and 256 global result slots. The native binding path is now explicit: `python tools/build_runtime.py --build t6mp_msstore --renderer-native` (or build_recon.py --build t6mp_msstore --enhanced --renderer-native). Without the option the original renderer routine is unchanged. The aliases in records/store-renderer-bindings.tsv are descriptive ABI names, not claims about original PDB symbols. The native source uses the original engine storage, enum tables, worker-name table, import cell and registry call boundaries, with the existing enhanced redirect mechanism. The renderer code is shared with the replay implementation. The binding audit verifies the exact original PE, 256 result slots, and Windows x64 object compilation using the real build_recon data-slot rewriting. Pending CI. This does not replace the original core registry yet or establish that the whole enhanced DLL links and runs in a live game.
