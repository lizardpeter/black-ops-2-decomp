# Microsoft Store BO2: true external-DLL reconstruction

## Goal and constraints

Reconstruct the **actual code** and reachable algorithms of the original Microsoft Store x64 `XCurl.dll` and `binkw64.dll`, not compatibility wrappers around FFmpeg, libcurl, or other implementations. Preserve relevant ABI, state, error behavior, threading, and multimedia semantics. Differential-test the reconstructed source against the original binaries before replacing either dependency.

This is **not done yet**. The shipped Store EXEs still import both DLLs at process startup. Reconstructing selected functions does not remove those PE imports; the enhanced loader must eventually redirect/remove the imports, or the fully rebuilt EXE must link replacements statically.

## Original files (never commit them)

| R2 key | Size | SHA-256 |
|---|---:|---|
| `bo2/DUMP/XCurl.dll` | 198,248 B | `65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551` |
| `bo2/DUMP/binkw64.dll` | 310,784 B | `10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17` |

Tools:

```sh
python tools/dlls/fetch.py
python tools/dlls/inventory.py originals/dlls/XCurl.dll originals/dlls/binkw64.dll --json originals/dlls/inventory.json
# Optional: add --clients originals/t6mp_msstore.exe originals/t6zm_msstore.exe originals/t6sp_msstore.exe
```

Fetch verifies SHA-256 and does **not execute** any file. Inventory uses the repository's `pefile` dependency and scans import names, exports and .pdata as data. Original binaries and generated inventories remain under ignored `originals/`.

## Initial measured scope

| | XCurl | Bink |
|---|---:|---:|
| Functions imported by each Store EXE | 16 | 19 |
| Named exported entry points | 61 | 77 |
| x64 unwind entries | 431 | 676 |

Unwind entries are **not** a complete unique-function count: inlined routines, leaf functions, thunks, code sections without unwind records, and chained unwind regions require analysis.

The 16 imports: `curl_global_init`, `curl_global_cleanup`, `curl_easy_init`, `curl_easy_cleanup`, `curl_easy_setopt`, `curl_easy_getinfo`, `curl_easy_strerror`, `curl_slist_append`, `curl_slist_free_all`, `curl_multi_init`, `curl_multi_cleanup`, `curl_multi_add_handle`, `curl_multi_remove_handle`, `curl_multi_perform`, `curl_multi_poll`, `curl_multi_info_read`.

The 19 imports: `BinkOpen`, `BinkClose`, `BinkDoFrame`, `BinkDoFrameAsync`, `BinkDoFrameAsyncWait`, `BinkNextFrame`, `BinkPause`, `BinkGetRects`, `BinkGetFrameBuffersInfo`, `BinkRegisterFrameBuffers`, `BinkGetError`, `BinkGetRealtime`, `BinkSetSoundTrack`, `BinkSetMemory`, `BinkSetIOSize`, `BinkControlBackgroundIO`, `BinkStartAsyncThread`, `BinkRequestStopAsyncThread`, `BinkWaitStopAsyncThread`.

## First transcribed source bodies

`reconstructed_dlls/bink/bink_small.cpp` contains four independently decoded **export function bodies** from the original machine code. These are *not yet differential-tested or integrated*.

| Function | Original RVA | Behavior observed in original bytes |
|---|---|---|
| `BinkSetMemory` | `0x1DA30` | Store allocator and deallocator pointers at RVAs `0x542A8` and `0x542B0` |
| `BinkSetIOSize` | `0x16840` | Store unsigned 32-bit argument at global RVA `0x570E0` |
| `BinkSetSoundTrack` | `0x16870` | Clamp unsigned count to 32, store it at `0x570B0`, copy that many 32-bit IDs to `0x57250` |
| `BinkRegisterFrameBuffers` | `0x17950` | If handle and buffer pointer are non-null, store the latter at handle offset `0xD0` |

Other initial observations: `BinkGetError` at `0x14C40` returns a pointer to a global at `0x31860` (bounds/writers still unknown). `BinkDoFrame` at `0x1B0C0` is a small dispatch stub, **not** the underlying frame decoder.

The original allocators and full Bink handle type are unresolved; the standalone source does not yet reproduce original process initialization.

## Steps to completion

1. Import these exact hashes into Ghidra and export decompilation/disassembly/function tables, finding leaves in addition to .pdata-covered functions.
2. Trace the full transitive call graphs from the 35 imported entry points, following indirect calls and noting unresolved edges.
3. Recover all state and types. For Bink, this explicitly includes the **Bink container, video entropy/transform/decode pipeline, audio codec, frame buffers, timing, background worker threads, and error handling**. No third-party decoder or stub returns.
4. Reconstruct every reachable function in readable source and differential-test semantics against the original DLL on Windows. Implement full import-compatible exports only when tested.
5. Test complete playback of the **44 `.bik` assets** already in R2, comparing decoded frames, audio, timing, seeks, and failure behavior. The other 81 videos being `.webm` does not eliminate Bink decoding.
6. Reconstruct XCurl's HTTP/multi-handle semantics, callbacks, progress, errors, and TLS interactions. Separately determine which calls to decommissioned vendor services should be *intentionally removed from engine callers*, without confusing removal with faithful decompilation.
7. Only once the transitive set is fully matched, redirect/remove the Store executable's two DLL dependencies and verify cold startup, videos, menu flow, and all three game modes.

**Current status:** 13 Bink export bodies and two XCurl export caller bodies transcribed. The XCurl callers still need their original internal allocation callees. No end-to-end decoder, HTTP implementation, differential DLL harness, or DLL-free build exists.

## Reconstruction pass, 2026-11-23

These functions were decoded by disassembling bytes extracted from the SHA-256-pinned original DLLs; they are not copied from third-party library implementations.

### Additional Bink leaf bodies (`reconstructed_dlls/bink/bink_leaf.cpp`)

| Function | RVA | Machine-code behavior |
|---|---|---|
| `BinkSetError` | `14C00` | Copy up to 254 characters and NUL-terminate buffer at `31860` |
| `BinkGetError` | `14C40` | Return pointer to buffer at `31860` |
| `BinkSetFrameRate` | `16820` | Save two uint32 words at `570E4` and `311C0` |
| `BinkSetFileOffset` | `16830` | Save uint64 at `57128` |
| `BinkSetSimulate` | `16860` | Save uint32 at `570DC` |
| `BinkSetWillLoop` | `1DA40` | Toggle bit `0x80` of uint32 at handle+`0x20` |
| `BinkSetVideoOnOff` | `1D820` | Set handle+`0x134`, return input state |
| `BinkGetTrackID` | `1D320` | Read indexed uint32 from pointer at handle+`0x1D0` |
| `BinkGetTrackType` | `1D2E0` | Read indexed uint32 from pointer at handle+`0x1C8` |

Together with the first four setter bodies, **13 Bink exports** are transcribed. Five are BO2 imports. All 13 need differential verification; original global initialization and downstream consumers remain unresolved. Standalone sanitizer-enabled tests passed locally.

### XCurl caller transcriptions (`reconstructed_dlls/xcurl/slist.cpp`)

- `curl_slist_append` at `1BB80`: duplicates a string through an original function pointer at RVA `28020`, allocates a 16-byte node through internal function `932C` with tag 5, and appends.
- `curl_slist_free_all` at `1BC00`: walks nodes, frees the string if present, clears the pointer, then frees the node using internal function `9640` with tag 5.

**Important:** these two callers are *partial* source reconstructions because the original allocator functions and indirect target are not reconstructed. The isolated control-flow tests use test-double allocators only; they are not substitutes in the production implementation. Networking/TLS and every Bink decode routine remain unreconstructed.

### Analysis automation and tests

- [Manual Ghidra workflow](../.github/workflows/dll-ghidra.yml) exports full-DLL function inventories, disassembly, call graphs, and pseudocode for both SHA-256-pinned originals as CI artifacts. The workflow has been added, **not executed/verified**.
- [Standard CI](../.github/workflows/check.yml) now compiles and runs the Bink leaf and XCurl list tests using Clang with sanitizers.
- No original binaries are committed, and neither original EXE import table nor the enhanced loader was changed.

**Next:** run the Ghidra workflow, trace both `curl_slist` allocator callees and the call trees below `BinkOpen`/`BinkDoFrame`, then implement actual internal algorithms and differential test against pinned originals. No runtime replacement should be shipped until dependencies are fully reconstructed and matched.

## Full Ghidra artifacts and decoder-path triage (2026-11-23)

The [original-DLL Ghidra workflow](https://github.com/lizardpeter/black-ops-2-decomp/actions/runs/45226208577) **succeeded for both pinned DLLs**. Its two artifacts contain disassembly, pseudocode, function tables and call graphs. Ghidra identified 515 Bink functions (486 non-thunk decompilation records) and 525 XCurl functions (501 decompilation records). These are *analyzed functions*, not reconstructed C++ matches.

The new `tools/dlls/reachability.py` reads the original artifacts' `functions.tsv` and `callgraph.tsv`. The measured static reachability union of **all game-imported entry points** is 189/515 Bink functions and 204/525 XCurl functions. These counts are *not upper bounds*: indirect calls, callbacks, dynamic import resolution, and missing leaf edges can increase the actual implementation scope.

Key direct edges from the real decoder:

| Source | Original direct callees | Static transitive size |
|---|---|---:|
| `BinkOpen` (`0x19B80`) | `FUN_180017970` (initializer, 8,714-byte body) | 98 |
| `BinkDoFrame` (`0x1B0C0`) | tail-jump to `BinkDoFramePlane` at `0x1A930` | 71 |
| `BinkDoFramePlane` | `FUN_1800162D0` (I/O); `FUN_18001A260` (frame state); `FUN_18001A850` (audio handling); `FUN_1800274A0` (video decompression); `FUN_18001A380`; `RADTimerRead` | 70 |
| `BinkDoFrameAsync` | `FUN_180021F70` (worker dispatch), `FUN_18001A260`, `FUN_180001010`, `BinkSetError`, `RADTimerRead` | 18 |
| `curl_easy_init` | `FUN_18000DC94` and trace/allocator wrappers | 92 |
| `curl_multi_perform` | `FUN_180019B8C` and timing/lock/tracing wrappers | 134 |

**Prioritized decoder targets:** `FUN_180017970` for Bink header/open state, then `FUN_1800274A0` and `FUN_1800252C0` for core video decoding, and `FUN_18001A850` for audio. The presence of names and pseudocode does not imply a matched decoder.

### Additional reconstructed source, tests and boundaries

- `reconstructed_dlls/bink/bink_frames.cpp`: full instruction-derived body for BO2-imported `BinkGetFrameBuffersInfo` at RVA `17870`, including aligned YUV dimensions, descriptor flags, pointer clearing, and multi-buffer metadata copy. The matching standalone C++ test, extended original-Unicorn oracle, and a new `tests/dlls/differential_frames.py` compare byte-for-byte source/original outputs on deterministic edge and random cases. Results must be checked in the latest CI before declaring a differential match.
- `reconstructed_dlls/bink/bink_dispatch.cpp`: `BinkOpen`'s original one-shot global option snapshot/reset and `BinkDoFrame`'s tail dispatch constant `0x303`. Both accurately expose **unresolved original decoder callees**, `FUN_180017970` and `BinkDoFramePlane`; they are not working video functions. The 4-byte uninitialized original stack gap in the BinkOpen options is explicitly documented.
- `reconstructed_dlls/xcurl/duplicate_string.cpp`: internal `FUN_180017450` scans NUL, allocates exactly length+1 with allocation tag 5, then copies the string. The original `0x28020` indirect duplication slot points to this function. Internal allocator `FUN_18000932C` and copy `FUN_180004AD0` remain unresolved; tests use declared test fixtures only.

**Do not remove** either import yet: `XCurl.dll` request engine and Bink's audio/video decoding algorithms still require substantial reconstruction. The goal remains original machine-code-derived behavior, without FFmpeg/libcurl substitutes or silent no-op stubs.

## Verified follow-on reconstruction pass (2026-11-23)

The original binaries and full Ghidra outputs were used for this continuation. **No replacement libraries** or external decoder code were used.

### Additional original-code functions

| Module | Original RVA | New source | Coverage and limitations |
|---|---|---|---|
| Bink | `0x1B2B0` | [`bink_keyframe.cpp`](../reconstructed_dlls/bink/bink_keyframe.cpp) | Original `BinkGetKeyFrame`: backward/forward/nearest seeking, 1-based frames, skip-current flag. No callees; valid frame indexes only. |
| XCurl | `0x17910` | [`easy_strerror.cpp`](../reconstructed_dlls/xcurl/easy_strerror.cpp) | **BO2-imported** `curl_easy_strerror`: all 87 original explicit string cases and the unknown-error fallback, copied from original decompiled branch/string data. No library dependencies. |
| Bink | `0x2EF50` | [`transform_rotate.cpp`](../reconstructed_dlls/bink/transform_rotate.cpp) | Independent sibling to `0x2EE30`, used by `FUN_180028EC0` in Bink audio-transform path. |
| Bink | `0x2E630` | [`transform_butterfly16.cpp`](../reconstructed_dlls/bink/transform_butterfly16.cpp) | Independent 16-float transform used by `FUN_1800293C0`, `FUN_1800295A0` and several decoder routines. |

### Differential tests

All four functions have pinned-original-vs-native-C++ tests in the normal CI workflow. The new tests use the same x64 emulator harness as the game's existing differential tests and do not load the vendor DLL into the native test process.

- `tests/dlls/differential_keyframe.py`: **2,268** valid frame/flag seek cases plus a null-handle case, compared against the exact original export.
- `tests/dlls/differential_xcurl_strerror.py`: **295** error codes, including unmapped and negative values; compare exact returned strings.
- `tests/dlls/differential_rotate.py`: **384** test vectors per routine for both original coefficient-rotation kernels at `0x2EE30` and `0x2EF50`.
- `tests/dlls/differential_butterfly16.py`: **1,036** 16-float vectors across four coefficient values, compared byte-for-byte.

The GitHub Actions **DLL-specific steps** passed in [this run](https://github.com/lizardpeter/black-ops-2-decomp/actions/runs/45234572991). Any later failure in broader reconstruction CI must still be investigated separately; a passing differential set is not whole-game playback verification.

### Remaining dependency-removal gates

1. Follow Bink decoding beyond `FUN_1800293C0`/`FUN_1800295A0` into the large video and audio pipelines, including entropy decoding, transforms, YUV frame output, timing and async behavior; prove full video playback on real `.bik` assets.
2. Reconstruct XCurl `easy` and `multi` request handling, allocator callbacks, HTTP transport and TLS behavior; test actual BO2 callers.
3. Verify all 16 XCurl and 19 Bink imported symbols for the three Store executables, including their *transitive* dependencies, before changing PE imports or linking replacements into the enhanced executable.

**Not done:** either DLL elimination, original `BinkOpen` implementation, video decode, audio decode, or native-game end-to-end playback. Source reconstruction remains isolated from the working runtime.

## Reconstruction continuation: actual video inverse block + XCurl getinfo core (2026-11-23)

This pass uses the pinned original Ghidra disassembly and pseudocode. No FFmpeg, libcurl, or third-party codec source was imported. All new functions are intentionally isolated from the enhanced game build.

### Newly reconstructed Bink internals

| RVA | Source file | Transcribed behavior | Original-vs-source differential |
|---|---|---|---|
| `0x2A450` | [`audio_permute32.cpp`](../reconstructed_dlls/bink/audio_permute32.cpp) | 32-word bit-preserving sample permutation used by `FUN_1800293C0` | **518 vectors**, 128 bytes each |
| `0x2A570` | [`audio_permute32.cpp`](../reconstructed_dlls/bink/audio_permute32.cpp) | Related 32-word permutation used by `FUN_1800295A0` | **518 vectors**, 128 bytes each |
| `0x2F2E0` | [`video_inverse8x8.cpp`](../reconstructed_dlls/bink/video_inverse8x8.cpp) | Full original two-pass integer inverse transform from 64 quantized coefficients to 64 output pixels, including 32-bit signed wrap, shifts, constants and output stride | **624 blocks**, multiple coefficient distributions and 8/16/32-byte strides |

The video inverse transform is reachable from `FUN_1800252C0`, below the video decoding tree. It is one actual block-decoding primitive, **not** a full Bink codec. All 64 pixel bytes *and* row padding are compared against pinned-original instruction execution under Unicorn. The two permutations are compared on arbitrary raw float bit patterns (including NaNs and signed zero). These functions have no internal DLL callees.

**Next Bink video steps:** reconstruct entropy/bitstream parsing and coefficient production in `FUN_180024B80`, `FUN_180024FF0` and `FUN_1800252C0`; recover the other inverse-transform variants including `FUN_18002F630`; integrate block output into frame reconstruction, then prove actual `.bik` playback.

### Newly reconstructed XCurl internal code

| RVA | Source file | Transcribed behavior | Original-vs-source differential |
|---|---|---|---|
| `0xAD58` | [`getinfo_unlocked.cpp`](../reconstructed_dlls/xcurl/getinfo_unlocked.cpp) | Original `curl_easy_getinfo` inner option selection: 17 recognized option codes, exact struct offsets and 32-/64-bit and double conversion semantics, plus unknown-option `0x30` result | **399** handle/option combinations |

This code covers only the field-selector body. The original `FUN_18000AD58` surrounds its selection logic with Windows critical-section operations and conditional ETW tracing. The differential harness models critical-section imports as no-ops while inspecting result/memory behavior; those synchronization and profiling dependencies are **not decompiled or replaced**. `curl_easy_getinfo` itself is still not a complete reconstructed public export.

**Next XCurl steps:** reconstruct the allocator callback implementations and network/multi-handle state machine, then follow the actual BO2 call paths and identify service operations that can be intentionally removed rather than disguised as faithful decompilation.

### Test evidence and remaining constraints

The original-vs-source comparison steps for these three routines passed in [GitHub Actions run 45236933819](https://github.com/lizardpeter/black-ops-2-decomp/actions/runs/45236933819). The pre-existing full game differential and enhanced-runtime checks are additional, separate stages whose final result should be verified before declaring the entire workflow green.

**Not complete:** full native Bink video/audio playback, XCurl HTTP+TLS, or removing either PE import dependency. The Store EXEs are still linked to both original DLLs.

## Continued Bink video decoder work: 2x blocks, byte interleaver and prediction (2026-11-23)

All routines below are reconstructed from the original pinned Store x64 `binkw64.dll` machine code and Ghidra pseudocode. They are original-code source translations, **not wrappers around a codec library**.

| Original RVA | Source | Reconstruction state | Verified differential cases |
|---|---|---|---:|
| `0x2F630` | [`video_inverse8x8_2x.cpp`](../reconstructed_dlls/bink/video_inverse8x8_2x.cpp) | **Complete original inner routine**: reuse our previously verified 8×8 quantized inverse transform, then duplicate each output byte to a 2×2 block. Writes 16×16 pixels, retaining output stride and padding. | **456** |
| `0x237B0` | [`video_interleave_bytes.cpp`](../reconstructed_dlls/bink/video_interleave_bytes.cpp) | **Complete original inner routine**: interleave two fixed-length byte streams using the original 31-bit-truncated selection mask, append the unconsumed other stream, and return the count of mask-controlled selections. | **450** |
| `0x2F9F0` | [`video_predict8x8_core.cpp`](../reconstructed_dlls/bink/video_predict8x8_core.cpp) | **Arithmetic stage matched**: reconstruct 8×8 residual through our validated integer inverse transform, then add it to the 8×8 prediction/reference frame modulo 256. Native API currently accepts the quantization matrix explicitly; the original six-argument function instead looks up a table at RVA `0x5C120 + quant_index * 0x100`. **The full original ABI and embedded lookup tables remain TODO.** | **630** |

The exact original-vs-native tests are in
[`differential_video_inverse8x8_2x.py`](../tests/dlls/differential_video_inverse8x8_2x.py),
[`differential_video_interleave.py`](../tests/dlls/differential_video_interleave.py),
and [`differential_video_predict8x8.py`](../tests/dlls/differential_video_predict8x8.py). The last test obtains the quantization matrices from `Image.data` of the SHA-256-pinned DLL, rather than relying on assumed/hardcoded matrix values. The relevant CI steps passed in [workflow 45240459153](https://github.com/lizardpeter/black-ops-2-decomp/actions/runs/45240459153). These are **1,536 individual new differential comparisons**, not 1,536 decoded video frames.

### Immediate decoder work remaining

The contiguous core of video decoding remains in `FUN_1800252C0`, with entropy/bitstream routines `FUN_180023840`, `FUN_180024140`, `FUN_180024980`, `FUN_180024B80`, `FUN_180024DB0`, and `FUN_180024FF0`. `FUN_18002F9F0` now has matched arithmetic but needs the complete original quantization-table/index wrapper. This code must be reconstructed and validated using actual compressed frames before the 44 original `.bik` assets can play without `binkw64.dll`.

For XCurl, original `FUN_18000932C` and `FUN_180009640` are allocator/deallocator dispatchers invoking callbacks at `PTR_FUN_180028018` and `PTR_FUN_180028008`, with conditional trace callbacks. These, the network request state machine, and all callback ownership/lifetime rules remain unimplemented. No no-op replacement or generic allocator should be passed off as a faithful reconstruction.

**Neither original DLL was removed from the EXE import table, and the enhanced runtime was deliberately left unchanged.**

## Bink video entropy reconstruction: three matched routines (2026-11-23)

This continuation advanced from already-verified pixel transforms into **actual source-level bitstream entropy decoding**. The evidence is the pinned Store x64 `binkw64.dll` machine code (SHA-256 `10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17`), its full Ghidra output, and original-versus-recompiled differential tests. No Bink/FFmpeg/other decoder source or preexisting codec implementation is used.

### Original memory layouts

[`entropy_bytes.h`](../reconstructed_dlls/bink/entropy_bytes.h) captures the original `0x48`-byte decoder record: current and end pointers at +`0x00`/+`0x08`, 16-byte decoded Huffman symbol map at +`0x18`, lookahead width at +`0x28`, packed Huffman lookup pointer at +`0x30`, output-count bit width at +`0x38`, capacity at +`0x3C`, and buffer at +`0x40`. The bitstream record is `0x18` bytes: cached 32-bit value at +`0x00`, next word pointer at +`0x08`, and available-bit count at +`0x10`.

[`entropy_bit_cursor.h`](../reconstructed_dlls/bink/entropy_bit_cursor.h) implements the original little-endian/LSB-first 32-bit word refill **and its 64-bit temporary accumulator**. Differential testing caught a premature 32-bit truncation at a word boundary and a premature next-word advance when Huffman lookahead examined but did not consume that word; both were corrected. The pointer, low-32-bit cache, and bit count are now directly compared with the original.

### Reconstructed inner entropy functions

| Original Bink RVA | Reconstructed source | Verified source behavior | Pinned-original differential cases |
|---|---|---|---:|
| `0x24980` | [`entropy_bytes.cpp`](../reconstructed_dlls/bink/entropy_bytes.cpp) | Bit-coded byte count; one-bit repeated-literal mode with four-bit literal, otherwise a byte-per-Huffman-code lookup using packed code length and symbol map; preserve source bitreader state | **444** |
| `0x24B80` | [`entropy_nibbles.cpp`](../reconstructed_dlls/bink/entropy_nibbles.cpp) | Bit-coded byte count; decode two independent Huffman nibbles, combine as `low | (high << 4)`; preserve bitreader state | **300** |
| `0x24DB0` | [`entropy_signed_bytes.cpp`](../reconstructed_dlls/bink/entropy_signed_bytes.cpp) | Bit-coded byte count; Huffman byte values plus sign bit when nonzero, or repeated 4-bit magnitude with optional sign bit; preserve bitreader state | **444** |

**1,188 source-vs-original comparisons passed** for these routines. The test inputs include zero/short/long output counts, 32-bit word boundaries, 4-bit fixed Huffman codes, mixed 1/2/3-bit prefix-free Huffman codes, positive/negative values, repeated literals, and zero values requiring no sign bit. The test files are [`differential_entropy_bytes.py`](../tests/dlls/differential_entropy_bytes.py), [`differential_entropy_nibbles.py`](../tests/dlls/differential_entropy_nibbles.py), and [`differential_entropy_signed.py`](../tests/dlls/differential_entropy_signed.py). They run the **actual original instructions in Unicorn** with constructed valid original-layout tables and buffers, and compare decoded bytes, original state pointers, and bit-reservoir fields against separately compiled C++.

The DLL-specific checks succeeded in [workflow 45246173263](https://github.com/lizardpeter/black-ops-2-decomp/actions/runs/45246173263). That does not imply the entire enhanced-game workflow finished, nor that the original binary has been replaced.

### Scope boundaries and next decoder targets

- **Validated:** these three specific entropy functions on their synthetic, well-formed test matrix. Their original memory layouts and exact word-consumption semantics are captured, with no external decoder dependency.
- **Not yet validated:** realistic Huffman tables initialized by the Bink stream header, real `.bik` compressed-frame input, malformed/corrupted streams, original `FUN_180023840` code-table initialization, the 16-bit coefficient/RLE bitstream routine `FUN_180024FF0`, and the full `FUN_1800252C0` frame/block decoder.
- **Next:** reconstruct Huffman table initialization and signed 16-bit delta/RLE entropy path, then pass real frame bitstreams through the matching original and reconstructed decoder state and compare reconstructed YUV frames.
- **No runtime integration yet:** both `binkw64.dll` and `XCurl.dll` remain required by the original Store EXE imports. The enhanced runtime has not been altered to load these incomplete replacements.

These source routines are internal decoder components, not a completed Bink decoder or executable-compatible replacement DLL.

## Original Bink coefficient and Huffman/RLE stream decoders — continuation (2026-11-23)

This pass used the **original Ghidra pseudocode and instruction-level x64 behavior** for two additional video-decoding routines, not a guessed codec or a substitute multimedia library. The source stays separate from the enhanced runtime.

| Original RVA | Reconstructed source | What it actually does | SHA-pinned original-versus-source checks |
|---|---|---|---:|
| `0x24FF0` | [`entropy_deltas16.cpp`](../reconstructed_dlls/bink/entropy_deltas16.cpp) | Decodes a bounded **16-bit coefficient stream** with signed/unsigned starting values, groups of up to eight subsequent coefficients, four-bit group delta widths, zero-width repeated groups, optional sign bits for nonzero deltas and low-16-bit output wrapping. The original count limit is **output capacity in bytes divided by two**, not capacity in bytes. | **672** |
| `0x24140` | [`entropy_byte_rle.cpp`](../reconstructed_dlls/bink/entropy_byte_rle.cpp) | Huffman byte tokens; symbols 0–11 are literals; symbols 12–15 repeat the previous value **4, 8, 12, 32** times respectively. Also handles the distinct repeated-four-bit-literal mode. | **516** |

These are **1,188 new differential comparisons**. Both functions reuse the original-layout records and shared source-level `BitCursor`, including the distinction between lookahead and consumption. Their tests check output bytes, both buffer pointers and the cached bitstream/available-bit/next-word fields. They also independently encode the expected output for well-formed synthetic streams rather than relying only on reconstruction-versus-original agreement.

Source-level tests are [`test_entropy_deltas16.cpp`](../reconstructed_dlls/bink/test_entropy_deltas16.cpp) and [`test_entropy_byte_rle.cpp`](../reconstructed_dlls/bink/test_entropy_byte_rle.cpp); differential tests are [`differential_entropy_deltas16.py`](../tests/dlls/differential_entropy_deltas16.py) and [`differential_entropy_byte_rle.py`](../tests/dlls/differential_entropy_byte_rle.py). The dedicated steps passed in [workflow 45248746391](https://github.com/lizardpeter/black-ops-2-decomp/actions/runs/45248746391); full-game checks are separate.

### Original Huffman codebook metadata recovered

The **real pinned DLL's BINKCONS** section contains a complete inventory of *sixteen* precomputed codebook lookup tables, selected at the beginning of `FUN_180023840`:

- Table-width bytes: RVA `0x5A740`, **[4, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7]**.
- 16 little-endian 32-bit offsets at RVA `0x5A800`, into a common table base at RVA `0x5AD40`: **[0, 16, 48, 80, 112, 144, 176, 208, 272, 336, 400, 464, 528, 592, 720, 848]**.
- Each type has exactly `1 << width` packed byte entries, for **976 lookup entries total**. Each entry encodes its Huffman code length in the high nibble and symbol-map index in the low nibble. Type 0 has 16 entries `0x40` through `0x4F` (fixed four-bit codes); the other types include 1–7-bit codes.
- The RLE lengths above are the exact bytes at RVA `0x5A690`–`0x5A693`, referenced by the original `FUN_180024140` as `DAT_18005A684 + symbol`.

These metadata values were read from the SHA-256-pinned original DLL, **not invented**. They are now preserved byte-for-byte in [`original_huffman_tables.h`](../reconstructed_dlls/bink/original_huffman_tables.h) and used by the reconstructed `FUN_180023840`, verified against the original instructions.

### What remains before actual .bik playback

The codebook initializer `FUN_180023840` is now reconstructed and synthetically verified, including all sixteen original table types. The highest-value next target is **`FUN_1800252C0`** (large frame/block traversal and mode handling), alongside its remaining block-parameter and reconstruction helpers. Need to validate the initialized Huffman tables and the verified entropy routines against bitstreams extracted from *real* original BO2 `.bik` assets, then assemble YUV output and run end-to-end playback. The current differentials exercise intentionally constructed valid bitstreams, **not full video frames**, corrupted data, audio output, timing or asynchronous playback.

All three original Store EXEs still depend on `binkw64.dll` and `XCurl.dll`; their import tables remain unchanged. **Neither DLL has been eliminated.**

## Bink Huffman codebook initialization + chained decoding verified (2026-11-23)

**Original initializer:** `FUN_180023840` at RVA `0x23840` (2,297 original x64 bytes).
**Reconstruction:** [`huffman_codebook.cpp`](../reconstructed_dlls/bink/huffman_codebook.cpp), using exact
[original BINKCONS tables](../reconstructed_dlls/bink/original_huffman_tables.h), with standalone
[`test_huffman_codebook.cpp`](../reconstructed_dlls/bink/test_huffman_codebook.cpp).

- Original initializer's four arguments remain separate: 24-byte input bitstream, selected table pointer output, 32-bit table width output, and 16-byte symbol permutation output.
- All **16** original Huffman types now select the exact original lookup bytes (total **976**), original widths and offsets.
- Type 0 uses identity mapping. Nonzero types support (a) a 1–8-symbol explicit unique nibble prefix with remaining symbols in ascending order and (b) 2-bit-depth hierarchical symbol permutations: pair swaps followed by optional stable 2+2, 4+4, and 8+8 merges. These merges preserve the original `FUN_1800237B0` variable bit-consumption semantics (stop when one side is exhausted).
- [`differential_huffman_codebook.py`](../tests/dlls/differential_huffman_codebook.py) verified **1,936 original-vs-native cases** on the pinned x64 instructions. Checks include output mappings, actual table bytes, and bitreader cache, count and next-word pointer. The first unit-test run detected a C++ assignment-order indexing bug in the generic merge; it was fixed before the passing original differential run.
- [`differential_codebook_to_entropy.py`](../tests/dlls/differential_codebook_to_entropy.py) verifies a **chained** two-routine path: original codebook `0x23840` initialization, then original Huffman byte decoder `0x24980`, versus the corresponding separately compiled reconstructed sources. Uses actual codewords generated from all 16 original tables, plus independently calculated expected symbols. **558** valid synthetic compressed streams passed, including intermediate and final bitreader state and decoded bytes.
- Both CI steps passed in [workflow 45253171154](https://github.com/lizardpeter/black-ops-2-decomp/actions/runs/45253171154). That workflow's other, broader game checks are independent; do not infer end-to-end playback from these synthetic validations.

### Size and effort remaining

The original Store games' **19 imported Bink exports** reach approximately **189 distinct Ghidra-recognized functions** and **96,809 bytes** of machine code through static direct calls. The **16 imported XCurl exports** reach approximately **204 functions** and **61,829 bytes**. Those lower-bound figures omit callback, indirect and unrecognized code paths; they do **not** count required functions left to rewrite or represent completion percentages.

The biggest outstanding Bink functions include `FUN_1800252C0` (8,618 bytes, core video frame traversal), `FUN_180017970` (8,714 bytes, media/file opening), and audio/frame scheduling/async paths. XCurl's remaining network and multi-handle machinery also has significant transitive dependencies. A successful synthetic entropy block does **not** mean full video frame output is near completion: actual frame layouts, chroma, motion/prediction, audio, timing and buffer allocation still need original-source recovery and real-media testing.

The **next measurable milestone** is producing a byte-identical decoded YUV frame from a real pinned BO2 `.bik` media fixture through original and source implementations. A subsequent milestone is a complete playable clip without loading the original DLL, then removing PE imports after all 19 Bink entry points and their state/lifetime behavior are handled. XCurl requires a separate HTTP/TLS/multi-handle and callback reconstruction track. Neither DLL dependency has been removed.

## First real BO2 keyframe oracle + original coefficient source reconstruction (2026-11-23)

This work moved from synthetic bitstreams to an actual SHA256-pinned BO2 Store
`.bik` media fixture and full-frame execution of the **original** decoder. It
also source-reconstructed another complex, frame-critical routine.

### Original game video and luma oracle

- Fixture: `la_pip_seq_6.bik` SHA-256
  `c1171207707c3cdcc44160899efbdc7016b1df94b21d11c592546983dd15049c`,
  256 × 256 pixels, 150 frames.
- First compressed frame starts at byte 648 and is 1,676 bytes long.
  Its 4-byte luma-plane partition offset is **1,372**, meaning 1,368 bytes of
  compressed luma payload after the partition header.
- [`tools/dlls/probe_original_bik_frame.py`](../tools/dlls/probe_original_bik_frame.py)
  calls original `FUN_1800252C0` through Unicorn with original caller-derived
  scratch buffers and real compressed bytes. The original function returned
  the **end of the luma payload** and produced 65,536 modified Y-plane bytes.
  Luma SHA-256: **`347b7e90cad7c4cc5d0e3e9b0541813b3a49605827aa72ec59c662ff1f24edaf`**.
  It saves a lossless `original-bink-luma-frame0.pgm` reference artifact.
  **The ORIGINAL DLL produced this image, not the native source frame decoder.**
- The original luma trace reached 23 Huffman codebook setups, 65 coefficient
  decodes, 36 2× inverse transforms, 29 8×8 inverse transforms, and the
  original byte and signed/unsigned adaptive entropy paths.
- [`differential_real_bik_codebooks.py`](../tests/dlls/differential_real_bik_codebooks.py)
  runs original versus source `FUN_180023840` on **23 sequential codebooks
  from each of three real BO2 video keyframes**. All **69** codebooks matched
  table bytes, symbol mappings and bitstream state; unlike prior synthetic
  tests these bytes came directly from the original compressed video data.

### Native source-level coefficient block restored

- Original `FUN_18002FD80` at RVA **`0x2FD80`** is a 2,193-byte x64
  hierarchical bit-plane coefficient decoder.
- [`video_coefficients.cpp`](../reconstructed_dlls/bink/video_coefficients.cpp)
  reconstructs its original algorithm using a token queue with group
  expansion, signed bit-plane magnitudes and the exact original 64-index
  coefficient permutation. The caller's first 16-bit DC coefficient stays
  untouched. The original 24-byte WordBitStream ABI and refill semantics
  are preserved through the existing verified `BitCursor`.
- [`capture_original_bik_coefficients.py`](../tools/dlls/capture_original_bik_coefficients.py)
  records the initial and final **128-byte coefficient block and 24-byte
  bitreader state for all 65 actual calls** from the successfully decoded
  original first luma frame, together with the entire frame packet.
  The captured corpus is an ephemeral CI artifact and **is not checked in**
  with the proprietary original media.
- [`replay_real_coefficient_corpus.py`](../tests/dlls/replay_real_coefficient_corpus.py)
  independently replays those 65 records through the original x64 function
  under Unicorn. All 65 exact output/state replays passed.
- [`differential_real_coefficients.py`](../tests/dlls/differential_real_coefficients.py)
  calls the pinned original function and the independently compiled
  reconstructed C++ with matching real media inputs for each captured block;
  **all 65 actual compressed-data blocks matched, byte-for-byte**, including
  all 128 coefficient bytes and the 24-byte bitreader state after relocation.
  The same source model was separately checked locally before committing it.
- The 65 real blocks contain 4,160 total coefficient positions and have
  1,487 nonzero decoded coefficients. The original decoder consumes
  7,534 bits across those coefficient calls (11–333 bits per block).
  Thus the test has nontrivial sparse/sign/bit-plane cases, not only zero
  blocks.

### Quantized prediction now has its original ABI and tables

Previously the reconstructed core at `0x2F9F0` required its caller to pass
the original quantization table. We extracted and committed **all 16 pinned
original 8×8 int32 matrices** (4,096 original bytes at RVA `0x5C120`) in
[`original_quant8x8.h`](../reconstructed_dlls/bink/original_quant8x8.h)
and wrapped the tested arithmetic in
[`video_predict8x8.cpp`](../reconstructed_dlls/bink/video_predict8x8.cpp).
The six-argument original-compatible internal function passed **1,680**
original-vs-C++ comparisons (16 table indices × 35 coefficient inputs ×
three reference/output stride combinations), in addition to the existing
explicit-table arithmetic comparison.

### Original adaptive entropy pairs restored

[`entropy_adaptive_pairs.cpp`](../reconstructed_dlls/bink/entropy_adaptive_pairs.cpp)
reconstructs two original decoder routines, `0x243B0` (signed two-nibble
adaptation) and `0x246A0` (unsigned), with the original adaptive record
layout, symbol-map and table-pointer arrays, current-book updates, repeat
mode, and sign conversion. A total **600** original-versus-source
comparisons passed in
[`differential_entropy_adaptive_pairs.py`](../tests/dlls/differential_entropy_adaptive_pairs.py).
This is functionality actually reached by the first real Bink frame.

### Remaining main first-frame source block

The successful original luma frame trace gives us a concrete first-frame
vertical slice. Its major decoding helper and entropy call sites have now
been independently reconstructed and compared with the original, including
the coefficient bit-plane function. The **remaining central source-port
target is `FUN_1800252C0` (8,618 machine-code bytes)**, which orchestrates
tile/block iteration, buffer readers, prediction and placement. It must be
reconstructed and its 65,536-byte luma output compared against the original
Y SHA-256 above.

Passing the first **Y plane** will not yet implement full color frames:
chroma plane orchestration, additional block modes, subsequent dependent
frames, playback and timing, audio, and game integration remain separate
targets. In particular, original `binkw64.dll` is still required by the
Store game's external imports; removing `XCurl.dll` remains a completely
separate large HTTP/TLS/networking/state-machine task.

All new source/CI work is non-destructive and isolated from the enhanced
runtime until a complete replacement is actually validated.

## First complete real BO2 Bink color keyframe (2026-11-23)

The reconstructed native C++ plane coordinator now reproduces all three
planes of the first real `la_pip_seq_6.bik` keyframe, including the two
128×128 chroma planes. The original binary is used only for the independent
Unicorn reference decode in the differential test; it is not called by
the native source decoder.

[Source plane coordinator](../reconstructed_dlls/bink/video_luma_first_keyframe.cpp)
and [three-plane differential test](../tests/dlls/differential_first_color_keyframe.py).

| Plane | Dimensions | Exact matching bytes | Next packet offset | SHA-256 |
|---|---|---:|---:|---|
| Y | 256×256 | 65,536 | 1,372 | `347b7e90cad7c4cc5d0e3e9b0541813b3a49605827aa72ec59c662ff1f24edaf` |
| Chroma 1 | 128×128 | 16,384 | 1,580 | `da5d24dd5daf0d2204e312c198aae5ecf3fc5ce0a20c90e75f3725b20f0bc69c` |
| Chroma 2 | 128×128 | 16,384 | 1,676 | `b16cfbb69348d5676817b3e8c67e0019b8c8a45a322b40ceb835d5a71800fb6e` |

All **98,304 output bytes**, compressed-word cursors, and observed mode
and submode counts matched in [CI run 45304096171](https://github.com/lizardpeter/black-ops-2-decomp/actions/runs/45304096171).
Both implementations used identically initialized destination and reference
buffers. The original end-of-input/mode-stream guard leaves any skipped
destination blocks unchanged; the source now reproduces this behavior
rather than decoding remaining buffered tokens past the packet boundary.
The native first chroma cursor selects the second chroma input independently
of the oracle. The strict regression workflow saves the three native and
original planes and the comparison report as artifacts.

The original entropy contexts use width-dependent counts/capacities; the
source now computes these for the plane dimensions rather than assuming
a 256-pixel luma plane. The supported subset remains the observed keyframe
modes, not the entire original 0x252C0 routine. This is not a claim of
complete Bink playback or a hardened arbitrary-input parser.

The next target is dependent-frame decoding with actual previous-frame
pixels. [Frame-1 original probe](../tests/dlls/probe_next_bink_luma.py)
uses the verified native frame-0 luminance output as the reference and
records the original next frame's mode histogram and pixel oracle.
The remaining work includes interframe prediction and other block modes,
multi-frame/container integration, audio, public Bink API and lifetime
behavior, and finally safe BO2 runtime integration. Both external DLLs
remain required.
